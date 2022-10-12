/**********************************************************************
 * File : RTOSAnalysis.h
 * Copyright (c) zeus.
 * Created On : Mon Jul 11 2022
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/
#ifndef RTOSAnalysis_h
#define RTOSAnalysis_h
#include <stdint.h>
#include<stdbool.h>

void RTOSAnalysis_Init(uint32_t TaskStack,uint32_t TaskPriority,uint32_t logWin);

#endif /* RTOSAnalysis_h */
