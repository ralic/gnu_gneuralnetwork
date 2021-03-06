/* randomize.h -- This belongs to gneural_network

   gneural_network is the GNU package which implements a programmable neural network.

   Copyright (C) 2016-2017 Ray Dillinger    <bear@sonic.net>
   Copyright (C) 2016 Jean Michel Sellier   <jeanmichel.sellier@gmail.com>

   This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either version 3, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY-2017 or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.  If not, see
   <http://www.gnu.org/licenses/>.
*/

// assigns weights randomly for each neuron

#ifndef RANDOMIZE_H
#define RANDOMIZE_H

#include <network.h>

void randomize(network *, network_config*);

double randomfloat(const double min, const double max);

#endif
