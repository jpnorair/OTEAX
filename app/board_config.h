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
  * @file       /apps/demo_ponglt/code/board_config.h
  * @author     JP Norair (jpnorair@indigresso.com)
  * @version    V1.0
  * @date       31 July 2012
  * @brief      Board & HW Selection for Demo PongLT app
  *
  * Don't actually include this.  Include OT_platform.h instead.  This file and
  * others like it are important to the preprocessor-based configuration scheme.
  ******************************************************************************
  */

#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

#include <app/build_config.h>


///@todo this must be expanded to meet different builds of OTEAX

#include <board/stm32l1xx/board_null.h>



/// Macro settings: ENABLED, DISABLED, NOT_AVAILABLE
#ifdef ENABLED
#   undef ENABLED
#endif
#define ENABLED  1

#ifdef DISABLED
#   undef DISABLED
#endif
#define DISABLED  0

#ifdef NOT_AVAILABLE
#   undef NOT_AVAILABLE
#endif
#define NOT_AVAILABLE   DISABLED





#endif 
