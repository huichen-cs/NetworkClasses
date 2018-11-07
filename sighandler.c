/**
 Copyright (C) 2015 Hui Chen <huichen AT ieee DOT org>
 
 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Description */
/* 
 * define all signal related functions
 */

#include <signal.h>

#include "sighandler.h"

sighandler_t setupsignal(int signum, sighandler_t handler)
{
    struct sigaction new_act, old_act;

    new_act.sa_handler = handler;
    sigemptyset (&new_act.sa_mask);
    new_act.sa_flags = 0;

    if (sigaction(signum, &new_act, &old_act) == -1)
        return SIG_ERR;
    else
        return old_act.sa_handler;
}    

