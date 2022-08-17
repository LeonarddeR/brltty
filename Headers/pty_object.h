/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2022 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_PTY_OBJECT
#define BRLTTY_INCLUDED_PTY_OBJECT

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct PtyObjectStruct PtyObject;

extern PtyObject *ptyNewObject (void);
extern void ptyDestroyObject (PtyObject *pty);

extern const char *ptyGetPath (const PtyObject *pty);
extern int ptyGetMaster (const PtyObject *pty);

extern int ptyOpenSlave (const PtyObject *pty, int *fileDescriptor);
extern void ptyCloseMaster (PtyObject *pty);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BRLTTY_INCLUDED_PTY_OBJECT */