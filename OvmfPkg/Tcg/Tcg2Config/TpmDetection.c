/** @file
  TPM2.0 auto detection.

Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
Copyright (C) 2018, Red Hat, Inc.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <PiPei.h>

#include <Library/DebugLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Tcg/Tcg2Config/Tcg2ConfigNvData.h>

/**
  This routine check both SetupVariable and real TPM device, and return final TpmDevice configuration.

  @param  SetupTpmDevice  TpmDevice configuration in setup driver

  @return TpmDevice configuration
**/
UINT8
DetectTpmDevice (
  IN UINT8 SetupTpmDevice
  )
{
  EFI_STATUS                        Status;

  DEBUG ((EFI_D_INFO, "DetectTpmDevice:\n"));

  Status = Tpm2RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    return TPM_DEVICE_NULL;
  }

  return TPM_DEVICE_2_0_DTPM;
}
