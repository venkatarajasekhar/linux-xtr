/*
 * Copyright 2002 Momentum Computer
 * Author: Matthew Dharm <mdharm@momenco.com>
 *
 * Copyright (C) 2003 Ralf Baechle (ralf@linux-mips.org)
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/config.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <asm/mv64340.h>

/*
 * pci_range_ck -
 *
 * Check if the pci device that are trying to access does really exists
 * on the evaluation board.  
 *
 * Inputs :
 * bus - bus number (0 for PCI 0 ; 1 for PCI 1)
 * dev - number of device on the specific pci bus
 *
 * Outpus :
 * 0 - if OK , 1 - if failure
 */
static inline int pci_range_ck(unsigned char bus, unsigned char dev)
{
	/* Accessing device 31 crashes the MV-64340. */
	if (dev < 5)
		return 0;
	return -1;
}

/*
 * galileo_pcibios_(read/write)_config_(dword/word/byte) -
 *
 * reads/write a dword/word/byte register from the configuration space
 * of a device.
 *
 * Note that bus 0 and bus 1 are local, and we assume all other busses are
 * bridged from bus 1.  This is a safe assumption, since any other
 * configuration will require major modifications to the CP7000G
 *
 * Inputs :
 * bus - bus number
 * dev - device number
 * offset - register offset in the configuration space
 * val - value to be written / read
 *
 * Outputs :
 * PCIBIOS_SUCCESSFUL when operation was succesfull
 * PCIBIOS_DEVICE_NOT_FOUND when the bus or dev is errorneous
 * PCIBIOS_BAD_REGISTER_NUMBER when accessing non aligned
 */

static int mv64340_read_config(struct pci_bus *bus, unsigned int devfn, int reg,
	int size, u32 * val)
{
	uint32_t address_reg, data_reg;
	uint32_t address;
	int bus = bus->number;
	int dev = PCI_SLOT(devfn);
	int func = PCI_FUNC(devfn);

	/* verify the range */
	if (pci_range_ck(busno, dev))
		return PCIBIOS_DEVICE_NOT_FOUND;

	/* select the MV-64340 registers to communicate with the PCI bus */
	if (bus == 0) {
		address_reg = MV64340_PCI_0_CONFIG_ADDR;
		data_reg = MV64340_PCI_0_CONFIG_DATA_VIRTUAL_REG;
	} else {
		address_reg = MV64340_PCI_1_CONFIG_ADDR;
		data_reg = MV64340_PCI_1_CONFIG_DATA_VIRTUAL_REG;
	}

	address = (bus << 16) | (dev << 11) | (func << 8) |
		  (offset & 0xfc) | 0x80000000;

	/* start the configuration cycle */
	MV_WRITE(address_reg, address);

	switch (size) {
	case 1:
		MV_READ_8(data_reg + (offset & 0x3), val);
		break;

	case 2:
		MV_READ_16(data_reg + (offset & 0x3), val);
		break;

	case 4:
		MV_READ(data_reg, val);
		break;
	}

	return PCIBIOS_SUCCESSFUL;
}

static int mv64340_write_config(struct pci_bus *bus, unsigned int devfn,
	int reg, int size, u32 val)
{
	int dev, bus, func;
	uint32_t address_reg, data_reg;
	uint32_t address;

	bus = device->bus->number;
	dev = PCI_SLOT(device->devfn);
	func = PCI_FUNC(device->devfn);

	/* verify the range */
	if (pci_range_ck(bus, dev))
		return PCIBIOS_DEVICE_NOT_FOUND;

	/* select the MV-64340 registers to communicate with the PCI bus */
	if (bus == 0) {
		address_reg = MV64340_PCI_0_CONFIG_ADDR;
		data_reg = MV64340_PCI_0_CONFIG_DATA_VIRTUAL_REG;
	} else {
		address_reg = MV64340_PCI_1_CONFIG_ADDR;
		data_reg = MV64340_PCI_1_CONFIG_DATA_VIRTUAL_REG;
	}

	address = (bus << 16) | (dev << 11) | (func << 8) |
		(offset & 0xfc) | 0x80000000;

	/* start the configuration cycle */
	MV_WRITE(address_reg, address);

	switch (size) {
	case 1:
		/* write the data */
		MV_WRITE_8(data_reg + (offset & 0x3), val);
		break;

	case 2:
		/* write the data */
		MV_WRITE_16(data_reg + (offset & 0x3), val);
		break;

	case 4:
		/* write the data */
		MV_WRITE(data_reg, val);
		break;
	}

	return PCIBIOS_SUCCESSFUL;
}

struct pci_ops mv64340_pci_ops = {
	.read	= mv64340_read_config,
	.write	= mv64340_write_config
};
