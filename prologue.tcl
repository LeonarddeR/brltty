###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2021 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU Lesser General Public License, as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any
# later version. Please see the file LICENSE-LGPL for details.
#
# Web Page: http://brltty.app/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

source [file join [file dirname [info script]] "brltty-prologue.tcl"]
set sourceRoot [file normalize [file dirname [info script]]]

set documentationSubdirectory Documentation
set programsSubdirectory Programs
set toolsSubdirectory Tools

set tablesSubdirectory Tables
set textTablesSubdirectory [file join $tablesSubdirectory Text]
set contractionTablesSubdirectory [file join $tablesSubdirectory Contraction]
set attributesTablesSubdirectory [file join $tablesSubdirectory Attributes]
set keyboardTablesSubdirectory [file join $tablesSubdirectory Keyboard]
set inputTablesSubdirectory [file join $tablesSubdirectory Input]

set driversSubdirectory Drivers
set brailleDriversSubdirectory [file join $driversSubdirectory Braille]
set speechDriversSubdirectory [file join $driversSubdirectory Speech]
set screenDriversSubdirectory [file join $driversSubdirectory Screen]

proc setSourceRoot {} {
   if {![findContainingDirectory ::env(BRLTTY_SOURCE_ROOT) [pwd] [list brltty.pc.in]]} {
      semanticError "source tree not found"
   }
}

proc setBuildRoot {} {
   global initialDirectory scriptDirectory

   foreach directory [list $initialDirectory $scriptDirectory] {
      if {[findContainingDirectory ::env(BRLTTY_BUILD_ROOT) $directory [list brltty.pc]]} {
         return
      }
   }

   semanticError "build tree not found"
}

