/**
 * Copyright (c) 2013 Anup Patel.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @file cpu_vcpu_vfp.c
 * @author Anup Patel (anup@brainfault.org)
 * @author Sting Cheng (sting.cheng@gmail.com)
 * @brief Source file for VCPU VFP emulation
 */

#include <vmm_error.h>
#include <vmm_stdio.h>
#include <arch_regs.h>
#include <cpu_inline_asm.h>
#include <cpu_vcpu_switch.h>
#include <cpu_vcpu_vfp.h>

#include <arm_features.h>

void cpu_vcpu_vfp_save(struct vmm_vcpu *vcpu)
{
	struct arm_priv *p = arm_priv(vcpu);
	struct arm_priv_vfp *vfp = &p->vfp;

	/* Do nothing if:
	 * 1. VCPU does not have VFPv3 feature
	 */
	if (!arm_feature(vcpu, ARM_FEATURE_VFP3)) {
		return;
	}

	/* Low-level VFP register save */
	cpu_vcpu_vfp_regs_save(vfp);
}

void cpu_vcpu_vfp_restore(struct vmm_vcpu *vcpu)
{
	struct arm_priv *p = arm_priv(vcpu);
	struct arm_priv_vfp *vfp = &p->vfp;

	/* Do nothing if:
	 * 1. VCPU does not have VFPv3 feature
	 */
	if (!arm_feature(vcpu, ARM_FEATURE_VFP3)) {
		return;
	}

	/* Low-level VFP register restore */
	cpu_vcpu_vfp_regs_restore(vfp);
}

int cpu_vcpu_vfp_trap(struct vmm_vcpu *vcpu,
		      arch_regs_t *regs,
		      u32 il, u32 iss)
{
	/* For now we don't handle VFP trap so just return failure. */
	return VMM_EFAIL;
}

void cpu_vcpu_vfp_dump(struct vmm_chardev *cdev, struct vmm_vcpu *vcpu)
{
	u32 i;
	struct arm_priv_vfp *vfp = &arm_priv(vcpu)->vfp;

	/* Do nothing if:
	 * 1. VCPU does not have VFPv3 feature
	 */
	if (!arm_feature(vcpu, ARM_FEATURE_VFP3)) {
		return;
	}

	vmm_cprintf(cdev, "VFP Feature Registers\n");
	vmm_cprintf(cdev, " %11s=0x%08"PRIx32"         %11s=0x%08"PRIx32"\n",
		    "MVFR0_EL1", vfp->mvfr0,
		    "MVFR1_EL1", vfp->mvfr1);
	vmm_cprintf(cdev, " %11s=0x%08"PRIx32"\n",
		    "MVFR2_EL1", vfp->mvfr2);
	vmm_cprintf(cdev, "VFP System Registers\n");
	vmm_cprintf(cdev, " %11s=0x%08"PRIx32"         %11s=0x%08"PRIx32"\n",
		    "FPCR", vfp->fpcr,
		    "FPSR", vfp->fpsr);
	vmm_cprintf(cdev, " %11s=0x%08"PRIx32"\n",
		    "FPEXC32_EL2", vfp->fpexc32);
	vmm_cprintf(cdev, "VFP Data Registers");
	for (i = 0; i < 64; i++) {
		if (i % 2 == 0) {
			vmm_cprintf(cdev, "\n");
		}
		vmm_cprintf(cdev, " %9s%02d=0x%016"PRIx64,
				  "D", (i), vfp->fpregs[i]);
	}
	vmm_cprintf(cdev, "\n");
}

int cpu_vcpu_vfp_init(struct vmm_vcpu *vcpu)
{
	struct arm_priv *p = arm_priv(vcpu);
	struct arm_priv_vfp *vfp = &arm_priv(vcpu)->vfp;

	/* Clear VCPU VFP context */
	memset(vfp, 0, sizeof(struct arm_priv_vfp));

	/* If host HW does not have VFP (i.e. software VFP) then
	 * clear all VFP feature flags so that VCPU always gets
	 * undefined exception when accessing VFP registers.
	 */
	if (!cpu_supports_fpu()) {
		goto no_vfp_for_vcpu;
	}

	/* If Host HW does not support VFPv3 or higher then
	 * don't allow VFP access to VCPU using CPTR_EL2
	 */
	if (arm_feature(vcpu, ARM_FEATURE_VFP3)) {
		p->cptr &= ~CPTR_TFP_MASK;
	} else {
		goto no_vfp_for_vcpu;
	}

	/* Current strategy is to show VFP feature registers
	 * same as underlying Host HW so that Guest sees same
	 * VFP capabilities as Host HW.
	 */
	vfp->mvfr0 = mrs(mvfr0_el1);
	vfp->mvfr1 = mrs(mvfr1_el1);
	vfp->mvfr2 = mrs(mvfr2_el1);

	return VMM_OK;

no_vfp_for_vcpu:
	arm_clear_feature(vcpu, ARM_FEATURE_MVFR);
	arm_clear_feature(vcpu, ARM_FEATURE_VFP);
	arm_clear_feature(vcpu, ARM_FEATURE_VFP3);
	arm_clear_feature(vcpu, ARM_FEATURE_VFP4);
	return VMM_OK;
}

int cpu_vcpu_vfp_deinit(struct vmm_vcpu *vcpu)
{
	/* For now nothing to do here. */
	return VMM_OK;
}
