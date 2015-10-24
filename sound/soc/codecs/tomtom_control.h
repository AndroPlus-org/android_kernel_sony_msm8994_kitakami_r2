/*
 * Author: Tom G., <roboter972@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "wcd9330.h"

int tomtom_write(struct snd_soc_codec *codec, unsigned int reg,
					unsigned int value);
unsigned int tomtom_read(struct snd_soc_codec *codec,
					unsigned int reg);

void tomtom_control_probe(struct snd_soc_codec *codec_ptr);
