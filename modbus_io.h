/*

modbus_io.h - plugin for for MODBUS I/O extension of grblHAL

Copyright (c) 2024 Richard Toth

This plugin is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This plugin is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this plugin.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef _MBIO_H_
#define _MBIO_H_

#ifndef MBIO_WAIT_STEP 
    #define MBIO_WAIT_STEP 50.0f
#endif

typedef enum {
    MBIO_Idle = 0,
    MBIO_Command,
} mbio_response_t;

#endif