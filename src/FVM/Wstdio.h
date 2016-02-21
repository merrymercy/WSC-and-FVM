/****************************************************/
/* File: Wstdio.h                                   */
/* simulate standard input/output                   */
/****************************************************/

/*	                          LISENCE
 *   Copyright (C) 2012  Wudy
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WSTDIO_H
#define WSTDIO_H

#include <stdio.h>

char *getsn( char *buffer, int len );
void ResetIO( void );
void BigFont( void );

unsigned int WaitKey( void );

#endif