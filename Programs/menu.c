/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#undef CAN_GLOB
#if defined(HAVE_GLOB_H)
#define CAN_GLOB
#include <glob.h>

#elif defined(__MINGW32__)
#define CAN_GLOB
#include <io.h>

#else /* glob: paradigm-specific global definitions */
#warning file globbing support not available on this platform
#endif /* glob: paradigm-specific global definitions */

#include "log.h"
#include "file.h"
#include "menu.h"
#include "prefs.h"
#include "parse.h"

typedef struct {
  const char *directory;
  const char *extension;
  const char *pattern;
  char *initial;
  char *current;
  unsigned none:1;

#if defined(HAVE_GLOB_H)
  glob_t glob;
#elif defined(__MINGW32__)
  char **names;
  int offset;
#endif /* glob: paradigm-specific field declarations */

  const char **paths;
  int count;
  unsigned char setting;
  const char *pathsArea[3];
} FileData;

typedef struct {
  Menu *menu;
  unsigned opened:1;

  unsigned int total;
  unsigned int visible;
} SubmenuData;

struct MenuStruct {
  Menu *parent;

  struct {
    MenuItem *array;
    unsigned int size;
    unsigned int count;
    unsigned int index;
  } items;

  MenuItem *activeItem;
  char valueBuffer[0X10];
};

typedef struct {
  int (*beginItem) (MenuItem *item);
  void (*endItem) (MenuItem *item, int deallocating);
  void (*activateItem) (MenuItem *item);
  const char * (*getValue) (const MenuItem *item);
  const char * (*getComment) (const MenuItem *item);
} MenuItemMethods;

struct MenuItemStruct {
  Menu *menu;
  unsigned char *setting;                 /* pointer to current value */
  MenuString name;                      /* item name for presentation */

  const MenuItemMethods *methods;
  MenuItemTester *test;                     /* returns true if item should be presented */
  MenuItemChanged *changed;

  unsigned char minimum;                  /* lowest valid value */
  unsigned char maximum;                  /* highest valid value */
  unsigned char divisor;                  /* present only multiples of this value */

  union {
    const MenuString *strings;
    FileData *files;
    SubmenuData *submenu;
  } data;
};

static inline const char *
getLocalText (const char *string) {
  return (string && *string)? gettext(string): "";
}

static const char *
formatValue (Menu *menu, const char *format, ...) {
  {
    va_list arguments;

    va_start(arguments, format);
    vsnprintf(menu->valueBuffer, sizeof(menu->valueBuffer), format, arguments);
    va_end(arguments);
  }

  return menu->valueBuffer;
}

Menu *
newMenu (void) {
  Menu *menu = malloc(sizeof(*menu));

  if (menu) {
    menu->parent = NULL;
    menu->items.array = NULL;
    menu->items.size = 0;
    menu->items.count = 0;
    menu->items.index = 0;
    menu->activeItem = NULL;
  }

  return menu;
}

static int
beginMenuItem (MenuItem *item) {
  return !item->methods->beginItem || item->methods->beginItem(item);
}

static void
endMenuItem (MenuItem *item, int deallocating) {
  if (item->methods->endItem) item->methods->endItem(item, deallocating);
}

void
deallocateMenu (Menu *menu) {
  if (menu) {
    if (menu->items.array) {
      MenuItem *item = menu->items.array;
      const MenuItem *end = item + menu->items.count;

      while (item < end) endMenuItem(item++, 1);
      free(menu->items.array);
    }

    free(menu);
  }
}

Menu *
getMenuParent (const Menu *menu) {
  return menu->parent;
}

unsigned int
getMenuSize (const Menu *menu) {
  return menu->items.count;
}

unsigned int
getMenuIndex (const Menu *menu) {
  return menu->items.index;
}

MenuItem *
getMenuItem (Menu *menu, unsigned int index) {
  return (index < menu->items.count)? &menu->items.array[index]: NULL;
}

static MenuItem *
getSelectedMenuItem (Menu *menu) {
  return getMenuItem(menu, menu->items.index);
}

static int
testMenuItem (Menu *menu, unsigned int index, int all) {
  MenuItem *item = getMenuItem(menu, index);

  if (!item) return 0;
  if (all) return 1;
  if (!item->test) return 1;
  return item->test();
}

static inline int
testMenuItemActive (Menu *menu, unsigned int index) {
  return testMenuItem(menu, index, 0);
}

static inline int
testMenuItemVisible (Menu *menu, unsigned int index) {
  return testMenuItem(menu, index, prefs.showAllItems);
}

int
isSettableMenuItem (const MenuItem *item) {
  return !!item->setting;
}

const MenuString *
getMenuItemName (const MenuItem *item) {
  return &item->name;
}

const char *
getMenuItemValue (const MenuItem *item) {
  return item->methods->getValue(item);
}

const char *
getMenuItemComment (const MenuItem *item) {
  return item->methods->getComment? item->methods->getComment(item): "";
}

static MenuItem *
newMenuItem (Menu *menu, unsigned char *setting, const MenuString *name) {
  if (menu->items.count == menu->items.size) {
    unsigned int newSize = menu->items.size? (menu->items.size << 1): 0X10;
    MenuItem *newArray = realloc(menu->items.array, (newSize * sizeof(*newArray)));

    if (!newArray) {
      logMallocError();
      return NULL;
    }

    menu->items.array = newArray;
    menu->items.size = newSize;
  }

  {
    MenuItem *item = getMenuItem(menu, menu->items.count++);

    item->menu = menu;
    item->setting = setting;

    item->name.label = getLocalText(name->label);
    item->name.comment = getLocalText(name->comment);

    item->methods = NULL;
    item->test = NULL;
    item->changed = NULL;

    item->minimum = 0;
    item->maximum = 0;
    item->divisor = 1;

    return item;
  }
}

void
setMenuItemTester (MenuItem *item, MenuItemTester *handler) {
  item->test = handler;
}

void
setMenuItemChanged (MenuItem *item, MenuItemChanged *handler) {
  item->changed = handler;
}

static const char *
getValue_numeric (const MenuItem *item) {
  return formatValue(item->menu, "%u", *item->setting);;
}

static const MenuItemMethods menuItemMethods_numeric = {
  .getValue = getValue_numeric
};

MenuItem *
newNumericMenuItem (
  Menu *menu, unsigned char *setting, const MenuString *name,
  unsigned char minimum, unsigned char maximum, unsigned char divisor
) {
  MenuItem *item = newMenuItem(menu, setting, name);

  if (item) {
    item->methods = &menuItemMethods_numeric;
    item->minimum = minimum;
    item->maximum = maximum;
    item->divisor = divisor;
  }

  return item;
}

static const char *
getValue_strings (const MenuItem *item) {
  const MenuString *strings = item->data.strings;
  return getLocalText(strings[*item->setting - item->minimum].label);
}

static const char *
getComment_strings (const MenuItem *item) {
  const MenuString *strings = item->data.strings;
  return getLocalText(strings[*item->setting - item->minimum].comment);
}

static const MenuItemMethods menuItemMethods_strings = {
  .getValue = getValue_strings,
  .getComment = getComment_strings
};

static void
setMenuItemStrings (MenuItem *item, const MenuString *strings, unsigned char count) {
  item->methods = &menuItemMethods_strings;
  item->data.strings = strings;
  item->minimum = 0;
  item->maximum = count - 1;
  item->divisor = 1;
}

MenuItem *
newStringsMenuItem (
  Menu *menu, unsigned char *setting, const MenuString *name,
  const MenuString *strings, unsigned char count
) {
  MenuItem *item = newMenuItem(menu, setting, name);

  if (item) {
    setMenuItemStrings(item, strings, count);
  }

  return item;
}

MenuItem *
newBooleanMenuItem (Menu *menu, unsigned char *setting, const MenuString *name) {
  static const MenuString strings[] = {
    {.label=strtext("No")},
    {.label=strtext("Yes")}
  };

  return newEnumeratedMenuItem(menu, setting, name, strings);
}

static int
qsortCompare_fileNames (const void *element1, const void *element2) {
  const char *const *name1 = element1;
  const char *const *name2 = element2;
  return strcmp(*name1, *name2);
}

static int
beginItem_files (MenuItem *item) {
  FileData *files = item->data.files;
  int index;

  files->paths = files->pathsArea;
  files->count = ARRAY_COUNT(files->pathsArea) - 1;
  files->paths[files->count] = NULL;
  index = files->count;

#ifdef CAN_GLOB
  {
#ifdef HAVE_FCHDIR
    int originalDirectory = open(".", O_RDONLY);

    if (originalDirectory != -1)
#else /* HAVE_FCHDIR */
    char *originalDirectory = getWorkingDirectory();

    if (originalDirectory)
#endif /* HAVE_FCHDIR */
    {
      if (chdir(files->directory) != -1) {
#if defined(HAVE_GLOB_H)
        memset(&files->glob, 0, sizeof(files->glob));
        files->glob.gl_offs = files->count;

        if (glob(files->pattern, GLOB_DOOFFS, NULL, &files->glob) == 0) {
          files->paths = (const char **)files->glob.gl_pathv;

          /* The behaviour of gl_pathc is inconsistent. Some implementations
           * include the leading NULL pointers and some don't. Let's just
           * figure it out the hard way by finding the trailing NULL.
           */
          while (files->paths[files->count]) files->count += 1;
        }
#elif defined(__MINGW32__)
        struct _finddata_t findData;
        long findHandle = _findfirst(files->pattern, &findData);
        int allocated = files->count | 0XF;

        files->offset = files->count;
        files->names = malloc(allocated * sizeof(*files->names));

        if (findHandle != -1) {
          do {
            if (files->count >= allocated) {
              allocated = allocated * 2;
              files->names = realloc(files->names, allocated * sizeof(*files->names));
            }

            files->names[files->count++] = strdup(findData.name);
          } while (_findnext(findHandle, &findData) == 0);

          _findclose(findHandle);
        }

        files->names = realloc(files->names, files->count * sizeof(*files->names));
        files->paths = files->names;
#endif /* glob: paradigm-specific field initialization */

#ifdef HAVE_FCHDIR
        if (fchdir(originalDirectory) == -1) logSystemError("fchdir");
#else /* HAVE_FCHDIR */
        if (chdir(originalDirectory) == -1) logSystemError("chdir");
#endif /* HAVE_FCHDIR */
      } else {
        logMessage(LOG_ERR, "%s: %s: %s",
                   gettext("cannot set working directory"), files->directory, strerror(errno));
      }

#ifdef HAVE_FCHDIR
      close(originalDirectory);
#else /* HAVE_FCHDIR */
      free(originalDirectory);
#endif /* HAVE_FCHDIR */
    } else {
#ifdef HAVE_FCHDIR
      logMessage(LOG_ERR, "%s: %s",
                 gettext("cannot open working directory"), strerror(errno));
#else /* HAVE_FCHDIR */
      logMessage(LOG_ERR, "%s", gettext("cannot determine working directory"));
#endif /* HAVE_FCHDIR */
    }
  }
#endif /* CAN_GLOB */

  qsort(&files->paths[index], files->count-index, sizeof(*files->paths), qsortCompare_fileNames);
  if (files->none) files->paths[--index] = "";
  files->paths[--index] = files->initial;
  files->paths += index;
  files->count -= index;
  files->setting = 0;

  for (index=1; index<files->count; index+=1) {
    if (strcmp(files->paths[index], files->initial) == 0) {
      files->paths += 1;
      files->count -= 1;
      break;
    }
  }

  for (index=0; index<files->count; index+=1) {
    if (strcmp(files->paths[index], files->current) == 0) {
      files->setting = index;
      break;
    }
  }

  item->maximum = files->count - 1;
  return 1;
}

static void
endItem_files (MenuItem *item, int deallocating) {
  FileData *files = item->data.files;

  if (files->current) free(files->current);
  files->current = deallocating? NULL: strdup(files->paths[files->setting]);

#if defined(HAVE_GLOB_H)
  if (files->glob.gl_pathc) {
    int i;

    for (i=0; i<files->glob.gl_offs; i+=1) files->glob.gl_pathv[i] = NULL;
    globfree(&files->glob);
    files->glob.gl_pathc = 0;
  }
#elif defined(__MINGW32__)
  if (files->names) {
    int i;

    for (i=files->offset; i<files->count; i+=1) free(files->names[i]);
    free(files->names);
    files->names = NULL;
  }
#endif /* glob: paradigm-specific memory deallocation */
}

static const char *
getValue_files (const MenuItem *item) {
  const FileData *files = item->data.files;
  const char *path;

  if (item == item->menu->activeItem) {
    path = files->paths[files->setting];
  } else {
    path = files->current;
  }

  if (!path) path = "";
  return path;
}

static const MenuItemMethods menuItemMethods_files = {
  .beginItem = beginItem_files,
  .endItem = endItem_files,
  .getValue = getValue_files
};

MenuItem *
newFilesMenuItem (
  Menu *menu, const MenuString *name,
  const char *directory, const char *extension,
  const char *initial, int none
) {
  FileData *files;

  if ((files = malloc(sizeof(*files)))) {
    char *pattern;

    memset(files, 0, sizeof(*files));
    files->directory = directory;
    files->extension = extension;
    files->none = !!none;

    {
      const char *strings[] = {"*", extension};
      pattern = joinStrings(strings, ARRAY_COUNT(strings));
    }

    if (pattern) {
      files->pattern = pattern; 

      if ((files->initial = *initial? ensureFileExtension(initial, extension): strdup(""))) {
        if ((files->current = strdup(files->initial))) {
          MenuItem *item = newMenuItem(menu, &files->setting, name);

          if (item) {
            item->methods = &menuItemMethods_files;
            item->data.files = files;
            return item;
          }

          free(files->current);
        } else {
          logMallocError();
        }

        free(files->initial);
      } else {
        logMallocError();
      }

      free(pattern);
    } else {
      logMallocError();
    }

    free(files);
  } else {
    logMallocError();
  }

  return NULL;
}

static MenuItem *
getParentMenuItem (const MenuItem *item) {
  return getSelectedMenuItem(item->menu->parent);
}

static int
beginItem_submenu (MenuItem *item) {
  item->data.submenu->visible = 0;

  {
    Menu *menu = item->data.submenu->menu;
    unsigned int size = getMenuSize(menu);
    unsigned int index;

    for (index=1; index<size; index+=1)
      if (testMenuItemActive(menu, index))
        item->data.submenu->visible += 1;

    item->data.submenu->total = size - 1;
  }

  return 1;
}

static void
endItem_submenu (MenuItem *item, int deallocating) {
  if (deallocating) {
    SubmenuData *submenu = item->data.submenu;

    deallocateMenu(submenu->menu);
    free(submenu);
  }
}

static void
activateItem_submenu (MenuItem *item) {
  endMenuItem(item, 0);
  item->data.submenu->opened = 1;
}

static const char *
getValue_submenu (const MenuItem *item) {
  return "--->";
}

static const char *
getComment_submenu (const MenuItem *item) {
  if (!prefs.showSubmenuSizes) return "";

  {
    const SubmenuData *submenu = item->data.submenu;

    return prefs.showAllItems?
             formatValue(item->menu, "%u", submenu->total):
             formatValue(item->menu, "%u/%u", submenu->visible, submenu->total);
  }
}

static const MenuItemMethods menuItemMethods_submenu = {
  .beginItem = beginItem_submenu,
  .endItem = endItem_submenu,
  .activateItem = activateItem_submenu,
  .getValue = getValue_submenu,
  .getComment = getComment_submenu
};

static void
activateItem_close (MenuItem *item) {
  item = getParentMenuItem(item);
  item->data.submenu->opened = 0;
  beginMenuItem(item);
}

static const char *
getValue_close (const MenuItem *item) {
  return getLocalText(strtext("Close"));
}

static const char *
getComment_close (const MenuItem *item) {
  return getLocalText(getMenuItemName(getParentMenuItem(item))->label);
}

static const MenuItemMethods menuItemMethods_close = {
  .activateItem = activateItem_close,
  .getValue = getValue_close,
  .getComment = getComment_close
};

Menu *
newSubmenuMenuItem (
  Menu *menu, const MenuString *name
) {
  SubmenuData *submenu;

  if ((submenu = malloc(sizeof(*submenu)))) {
    if ((submenu->menu = newMenu())) {
      static const MenuString closeName = {.label="<---"};
      MenuItem *close;

      if ((close = newMenuItem(submenu->menu, NULL, &closeName))) {
        MenuItem *item;

        if ((item = newMenuItem(menu, NULL, name))) {
          submenu->menu->parent = menu;
          submenu->opened = 0;
          close->methods = &menuItemMethods_close;

          item->methods = &menuItemMethods_submenu;
          item->data.submenu = submenu;
          return submenu->menu;
        }
      }

      deallocateMenu(submenu->menu);
    }

    free(submenu);
  } else {
    logMallocError();
  }

  return NULL;
}

int
setMenuPreviousItem (Menu *menu) {
  do {
    if (!menu->items.index) menu->items.index = menu->items.count;
    if (!menu->items.index) return 0;
  } while (!testMenuItemVisible(menu, --menu->items.index));

  return 1;
}

int
setMenuNextItem (Menu *menu) {
  if (menu->items.index >= menu->items.count) return 0;

  do {
    if (++menu->items.index == menu->items.count) menu->items.index = 0;
  } while (!testMenuItemVisible(menu, menu->items.index));

  return 1;
}

int
setMenuFirstItem (Menu *menu) {
  if (!menu->items.count) return 0;
  menu->items.index = 0;
  return testMenuItemVisible(menu, menu->items.index) || setMenuNextItem(menu);
}

int
setMenuLastItem (Menu *menu) {
  if (!menu->items.count) return 0;
  menu->items.index = menu->items.count - 1;
  return testMenuItemVisible(menu, menu->items.index) || setMenuPreviousItem(menu);
}

static int
activateMenuItem (MenuItem *item) {
  if (!item->methods->activateItem) return 0;
  item->methods->activateItem(item);
  return 1;
}

static int
adjustMenuItem (const MenuItem *item, void (*adjust) (const MenuItem *item)) {
  int count = item->maximum - item->minimum + 1;

  do {
    adjust(item);
    if (!--count) break;
  } while ((*item->setting % item->divisor) || (item->changed && !item->changed(item, *item->setting)));

  return !!count;
}

static void
decrementMenuItem (const MenuItem *item) {
  if ((*item->setting)-- <= item->minimum) *item->setting = item->maximum;
}

int
changeMenuItemPrevious (MenuItem *item) {
  if (activateMenuItem(item)) return 1;
  if (!item->setting) return 0;
  return adjustMenuItem(item, decrementMenuItem);
}

static void
incrementMenuItem (const MenuItem *item) {
  if ((*item->setting)++ >= item->maximum) *item->setting = item->minimum;
}

int
changeMenuItemNext (MenuItem *item) {
  if (activateMenuItem(item)) return 1;
  if (!item->setting) return 0;
  return adjustMenuItem(item, incrementMenuItem);
}

int
changeMenuItemScaled (MenuItem *item, unsigned int index, unsigned int count) {
  if (activateMenuItem(item)) return 1;

  if (item->setting) {
    unsigned char oldSetting = *item->setting;

    if (item->methods->getValue == getValue_numeric) {
      *item->setting = rescaleInteger(index, count-1, item->maximum-item->minimum) + item->minimum;
    } else {
      *item->setting = index % (item->maximum + 1);
    }

    if (!item->changed || item->changed(item, *item->setting)) return 1;
    *item->setting = oldSetting;
  }

  return 0;
}

MenuItem *
getCurrentMenuItem (Menu *menu) {
  MenuItem *newItem = getSelectedMenuItem(menu);
  MenuItem *oldItem = menu->activeItem;

  if (newItem != oldItem) {
    if (oldItem) endMenuItem(oldItem, 0);
    menu->activeItem = beginMenuItem(newItem)? newItem: NULL;
  }

  return newItem;
}

Menu *
getCurrentSubmenu (Menu *menu) {
  while (1) {
    MenuItem *item = getCurrentMenuItem(menu);

    if (item->methods != &menuItemMethods_submenu) break;
    if (!item->data.submenu->opened) break;
    menu = item->data.submenu->menu;
  }

  return menu;
}
