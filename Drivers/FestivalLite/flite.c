/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 2003 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* FestivalLite/speech.c - Speech library
 * For the Festival Lite text to speech package
 * Maintained by Mario Lang <mlang@delysid.org>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>

#include "Programs/spk.h"
#include "Programs/misc.h"

#define SPKNAME "FestivalLite"

#include "Programs/spk_driver.h"
#include <flite.h>
#include <flite_version.h>

extern	cst_voice	*REGISTER_VOX	(const char *voxdir);
extern	cst_voice	*UNREGISTER_VOX	(const cst_voice *voice);

static	cst_voice	*voice		= NULL;
static	cst_features	*features	= NULL;
static	const char	*outtype	= "play";

static	pid_t		child		= 0;

static void
spk_identify (void)
{
  LogPrint(LOG_NOTICE, "Using Festival Lite version %s-%s-%s %s.",
	   FLITE_PROJECT_PREFIX, FLITE_PROJECT_VERSION, FLITE_PROJECT_STATE,
	   FLITE_PROJECT_DATE);
}

static void
spk_open (char **parameters)
{
  features = new_features();
  flite_init();
  voice = REGISTER_VOX(NULL);
  feat_copy_into(features, voice->features);
  child = 0;
}

static void
spk_say (const unsigned char *buffer, int len)
{
  spk_mute();
  if ((child = fork()) == -1) {
    LogError("fork");
  } else if (child == 0) {
    flite_text_to_speech(buffer, voice, outtype);
    _exit(0);
  }
}

static void
spk_mute (void)
{
  if (child > 0) {
    kill(child, SIGKILL);
    child = 0;
  }
}

static void
spk_close (void)
{
  spk_mute();
  if (features) {
    delete_features(features);
    features = NULL;
  }
  UNREGISTER_VOX(voice);
}
