/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape Portable Runtime (NSPR).
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nspr.h"
#include "prinrval.h"
#include "plgetopt.h"
#include "pprthred.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


PRMonitor *mon;
#define THREAD_COUNT 2
PRInt32 alive;

void
PR_CALLBACK
DumbThread(void *arg)
{
    
    /* Nick - Time Action */
    PRTime t = 1234567890;
    PRExplodedTime et;
    PR_ExplodeTime(t, PR_LocalTimeParameters, &et);
    /* Nick - Done */

    PR_EnterMonitor(mon);
    alive--;
    PR_Notify(mon);
    PR_ExitMonitor(mon);
}

static void CreateThreads(PRThreadScope scope1, PRThreadScope scope2)
{
    PRThread *thr;
    
    alive = THREAD_COUNT;
    mon = PR_NewMonitor();
    
    int n;
    for (n = 0; n < THREAD_COUNT; n++) {
        thr = PR_CreateThread(PR_USER_THREAD,
                              DumbThread, 
                              (void *)scope2, 
                              PR_PRIORITY_NORMAL,
                              scope1,
                              PR_UNJOINABLE_THREAD,
                              0);
        assert(thr != 0);
        PR_Sleep(0);
    }

    /* Wait for all threads to exit */
    PR_EnterMonitor(mon);
    while (alive) {
        PR_Wait(mon, PR_INTERVAL_NO_TIMEOUT);
    }

    PR_ExitMonitor(mon);
    PR_DestroyMonitor(mon);
}

int main(int argc, char **argv) {

    PR_STDIO_INIT();
    PR_Init(PR_USER_THREAD, PR_PRIORITY_HIGH, 0);

    CreateThreads(PR_GLOBAL_THREAD, PR_GLOBAL_THREAD);
    
    PR_Cleanup();
    
    return 0;
}
