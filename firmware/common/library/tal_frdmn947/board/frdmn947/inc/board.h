/*
 * Copyright 2022-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _BOARD_H_
#define _BOARD_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief The ENET PHY address. */
#define BOARD_ENET0_PHY_ADDRESS (0x00U) /* Phy address of enet port 0. */

/*! @brief Memory ranges not usable by the ENET DMA. */
#ifndef BOARD_ENET_NON_DMA_MEMORY_ARRAY
#define BOARD_ENET_NON_DMA_MEMORY_ARRAY                                                     \
    {                                                                                       \
        {0x00000000U, 0x0007FFFFU}, {0x10000000U, 0x17FFFFFFU}, {0x80000000U, 0xDFFFFFFFU}, \
            {0x00000000U, 0x00000000U},                                                     \
    }
#endif /* BOARD_ENET_NON_DMA_MEMORY_ARRAY */

#endif /* _BOARD_H_ */

/*** EOF ***/

