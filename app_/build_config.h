/* Copyright 2010-2012 JP Norair
  *
  * Licensed under the OpenTag License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  * http://www.indigresso.com/wiki/doku.php?id=opentag:license_1_0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  */
/**
  * @file       /apps/demo_ponglt/code/build_config.h
  * @author     JP Norair (jpnorair@indigresso.com)
  * @version    R100
  * @date       31 July 2012
  * @brief      Most basic list of constants needed to configure build
  *
  * Do not include this file.  Include OTAPI.h (or OT_config.h + OT_types.h)
  * for device-independent stuff, and OT_platform.h for device-dependent stuff.
  ******************************************************************************
  */

#ifndef __BUILD_CONFIG_H
#define __BUILD_CONFIG_H

#include <otsys/support.h>



/** Endian Configuration  <BR>
  * ========================================================================<BR>
  * OpenTag might be compiled on Big or Little Endian Platforms.  Endianness
  * will impact many aspects of the compilation.  Sometimes, the endianness is
  * defined in system headers or via the compiler.
  */
#if (!defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__))
#   define __LITTLE_ENDIAN__
//#   define __BIG_ENDIAN__
#endif




#define OS_FEATURE(VAL)                 DISABLED                // NO OS Featuresetting just yet
#define OS_FEATURE_MEMCPY               DISABLED                //  
#define OS_FEATURE_MALLOC               DISABLED





#endif 
