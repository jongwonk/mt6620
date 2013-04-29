/*******************************************************************************
* Copyright (c) 2013, MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License version 2 as published by the Free
* Software Foundation.
*
* Alternatively, this software may be distributed under the terms of BSD
* license.
********************************************************************************
*/

/*******************************************************************************
* THIS SOFTWARE IS PROVIDED BY MEDIATEK "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, NON-INFRINGEMENT OR FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL MEDIATEK BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************************
*/

/*! \file
\brief  Declaration of library functions

Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define CMB_STUB_LOG_INFO(fmt, arg ...) printk(KERN_INFO fmt, ## arg)
#define CMB_STUB_LOG_WARN(fmt, arg ...) printk(KERN_WARNING fmt, ## arg)
#define CMB_STUB_LOG_DBG(fmt, arg ...) printk(KERN_DEBUG fmt, ## arg)

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <linux/kernel.h>
#include <linux/module.h>

#include <mach/mtk_wcn_cmb_stub.h>

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

static wmt_aif_ctrl_cb cmb_stub_aif_ctrl_cb = NULL;
static wmt_func_ctrl_cb cmb_stub_func_ctrl_cb = NULL;
static CMB_STUB_AIF_X cmb_stub_aif_stat = CMB_STUB_AIF_0;

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*!
* \brief A registration function for WMT-PLAT to register itself to CMB-STUB.
*
* An MTK-WCN-CMB-STUB registration function provided to WMT-PLAT to register
* itself and related callback functions when driver being loaded into kernel.
*
* \param p_stub_cb a pointer carrying CMB_STUB_CB information
*
* \retval 0 operation success
* \retval -1 invalid parameters
*/
int mtk_wcn_cmb_stub_reg(P_CMB_STUB_CB p_stub_cb)
{
	if ((!p_stub_cb)
	    || (p_stub_cb->size != sizeof(CMB_STUB_CB))) {
		CMB_STUB_LOG_WARN
		    ("[cmb_stub] invalid p_stub_cb:0x%p size(%d)\n", p_stub_cb,
		     (p_stub_cb) ? p_stub_cb->size : 0);
		return -1;
	}

	CMB_STUB_LOG_DBG("[cmb_stub] registered, p_stub_cb:0x%p size(%d)\n",
			 p_stub_cb, p_stub_cb->size);

	cmb_stub_aif_ctrl_cb = p_stub_cb->aif_ctrl_cb;
	cmb_stub_func_ctrl_cb = p_stub_cb->func_ctrl_cb;

	return 0;
}

EXPORT_SYMBOL(mtk_wcn_cmb_stub_reg);

/*!
* \brief A unregistration function for WMT-PLAT to unregister from CMB-STUB.
*
* An MTK-WCN-CMB-STUB unregistration function provided to WMT-PLAT to
* unregister itself and clear callback function references.
*
* \retval 0 operation success
*/
int mtk_wcn_cmb_stub_unreg(void)
{
	cmb_stub_aif_ctrl_cb = NULL;
	cmb_stub_func_ctrl_cb = NULL;

	CMB_STUB_LOG_INFO("[cmb_stub] unregistered \n");	/* KERN_DEBUG */

	return 0;
}

EXPORT_SYMBOL(mtk_wcn_cmb_stub_unreg);

/* stub functions for kernel to control audio path pin mux */
int mtk_wcn_cmb_stub_aif_ctrl(CMB_STUB_AIF_X state, CMB_STUB_AIF_CTRL ctrl)
{
	int ret;

	if ((CMB_STUB_AIF_MAX <= state)
	    || (CMB_STUB_AIF_CTRL_MAX <= ctrl)) {
		CMB_STUB_LOG_WARN("[cmb_stub] aif_ctrl invalid (%d, %d)\n",
				  state, ctrl);
		return -1;
	}

/* avoid the early interrupt before we register the eirq_handler */
	if (cmb_stub_aif_ctrl_cb) {
		ret = (*cmb_stub_aif_ctrl_cb) (state, ctrl);
		CMB_STUB_LOG_INFO("[cmb_stub] aif_ctrl_cb state(%d->%d) ctrl(%d) ret(%d)\n", cmb_stub_aif_stat, state, ctrl, ret);	/* KERN_DEBUG */

		cmb_stub_aif_stat = state;
		return ret;
	} else {
		CMB_STUB_LOG_WARN("[cmb_stub] aif_ctrl_cb null \n");
		return -2;
	}
}

EXPORT_SYMBOL(mtk_wcn_cmb_stub_aif_ctrl);

void mtk_wcn_cmb_stub_func_ctrl(unsigned int type, unsigned int on)
{
	if (cmb_stub_func_ctrl_cb) {
		(*cmb_stub_func_ctrl_cb) (type, on);
	} else {
		CMB_STUB_LOG_WARN("[cmb_stub] func_ctrl_cb null \n");
	}
}

EXPORT_SYMBOL(mtk_wcn_cmb_stub_func_ctrl);

static int _mt_combo_plt_do_deep_idle(COMBO_IF src, int enter)
{
	int ret = -1;

	const char *combo_if_name[] = {
		"COMBO_IF_UART",
		"COMBO_IF_MSDC"
	};

	if (src != COMBO_IF_UART && src != COMBO_IF_MSDC) {
		CMB_STUB_LOG_WARN("src = %d is error\n", src);
		return ret;
	}

	if (src >= 0 && src < COMBO_IF_MAX) {
		CMB_STUB_LOG_INFO("src = %s, to enter deep idle? %d \n",
				  combo_if_name[src], enter);
	}

/*TODO: For Common SDIO configuration, we need to do some judgement between STP and WIFI
to decide if the msdc will enter deep idle safely*/

	switch (src) {
	case COMBO_IF_UART:
		if (enter == 0) {
/* UART controller shall wake up */
		} else {
/* UART controller can go sleep */
		}
		ret = 0;
		break;

	case COMBO_IF_MSDC:
		if (enter == 0) {
/* SDIO controller shall wake up */
		} else {
/* SDIO controller can go sleep */
		}
		ret = 0;
		break;

	default:
		ret = -1;
		break;
	}

	return ret;
}

int mt_combo_plt_enter_deep_idle(COMBO_IF src)
{
/* return 0; */
/* TODO: [FixMe][GeorgeKuo] handling this depends on common UART or common SDIO */
	return _mt_combo_plt_do_deep_idle(src, 1);
}

EXPORT_SYMBOL(mt_combo_plt_enter_deep_idle);

int mt_combo_plt_exit_deep_idle(COMBO_IF src)
{
/* return 0; */
/* TODO: [FixMe][GeorgeKuo] handling this depends on common UART or common SDIO */
	return _mt_combo_plt_do_deep_idle(src, 0);
}

EXPORT_SYMBOL(mt_combo_plt_exit_deep_idle);