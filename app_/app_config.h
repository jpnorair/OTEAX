/* Copyright 2010-2014 JP Norair
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
  * @file       /apps/demo_ponglt/code/app_config.h
  * @author     JP Norair (jpnorair@indigresso.com)
  * @version    R101
  * @date       31 Aug 2014
  * @brief      Application Configuration File for PongLT Demo
  *
  * In your application code, you could include this file as <app/app_config.h>
  * but this is not recommended.  Instead, use:
  *
  * #include <otstd.h>
  * or
  * #include <otsys/config.h>
  *
  * As these are tested to properly bundle everything properly through the 
  * compiler.
  *
  ******************************************************************************
  */

#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include <app/build_config.h>


#define OT_PARAM(VAL)       0   //OT_PARAM_##VAL
#define OT_FEATURE(VAL)     0   //OT_FEATURE_##VAL
#define M2_FEATURE(VAL)     0
#define M2_PARAM(VAL)       0


#endif 
