/* 
 * net.h
 *
 * Copyright (C) 2000 Marcus Metzler <marcus@convergence.de>
 *                  & Ralph  Metzler <ralph@convergence.de>
                      for convergence integrated media GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef _DVBNET_H_
#define _DVBNET_H_

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif


struct dvb_net_if {
	uint16_t pid;
	uint16_t if_num;
};


#define NET_ADD_IF                 _IOWR('o', 52, struct dvb_net_if)
#define NET_REMOVE_IF              _IO('o', 53)

#endif /*_DVBNET_H_*/
