/* Copyright 2010-2012 JP Norair
  *
  * Licensed under the OpenTag License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  * http://www.indigresso.com/wiki/doku.php?idopentag:license_1_0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  */
/**
  * @file       /app/isr_config_STM32L.h
  * @author     JP Norair (jpnorair@indigresso.com)
  * @version    R100
  * @date       10 December 2012
  * @brief      Null STM32L ISR Configuration
  *
  * 
  ******************************************************************************
  */

#if !defined(__ISR_CONFIG_STM32L_H) && defined(__STM32L__)
#define __ISR_CONFIG_STM32L_H


/// Cortex M3 NVIC specification.  For STM32L, it should take values 1, 2, 4, 
/// or 8, or it can be defined in compiler.
#ifndef __CM3_NVIC_GROUPS
#   define __CM3_NVIC_GROUPS        1
#endif


#endif
