/** @file
  The module entry point for Tcg2 configuration module.

Copyright (c) 2018, Red Hat, Inc.
Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <PiPei.h>

#include <Guid/TpmInstance.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Tcg/Tcg2Config/Tcg2ConfigNvData.h>

TPM_INSTANCE_ID  mTpmInstanceId[] = TPM_INSTANCE_ID_LIST;

CONST EFI_PEI_PPI_DESCRIPTOR gTpmSelectedPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiTpmDeviceSelectedGuid,
  NULL
};

EFI_PEI_PPI_DESCRIPTOR  mTpmInitializationDonePpiList = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gPeiTpmInitializationDonePpiGuid,
  NULL
};

/**
  This routine check both SetupVariable and real TPM device, and return final TpmDevice configuration.

  @param  SetupTpmDevice  TpmDevice configuration in setup driver

  @return TpmDevice configuration
**/
UINT8
DetectTpmDevice (
  IN UINT8 SetupTpmDevice
  );

/**
  The entry point for Tcg2 configuration driver.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCES             Convert variable to PCD successfully.
  @retval Others                 Fail to convert variable to PCD.
**/
EFI_STATUS
EFIAPI
Tcg2ConfigPeimEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  UINTN                           Size;
  EFI_STATUS                      Status;
  EFI_STATUS                      Status2;
  TCG2_CONFIGURATION              Tcg2Configuration;
  UINTN                           Index;
  UINT8                           TpmDevice;

  Tcg2Configuration.TpmDevice           = TPM_DEVICE_2_0_DTPM;

  DEBUG ((EFI_D_INFO, "OvmfPkg Tcg2ConfigPeimEntryPoint"));

  if (PcdGetBool (PcdTpmAutoDetection)) {
    TpmDevice = DetectTpmDevice (Tcg2Configuration.TpmDevice);
    DEBUG ((EFI_D_INFO, "TpmDevice final: %x\n", TpmDevice));
    if (TpmDevice != TPM_DEVICE_NULL) {
      Tcg2Configuration.TpmDevice = TpmDevice;
    }
  } else {
    TpmDevice = Tcg2Configuration.TpmDevice;
  }

  //
  // Convert variable to PCD.
  // This is work-around because there is no gurantee DynamicHiiPcd can return correct value in DXE phase.
  // Using DynamicPcd instead.
  //
  // NOTE: Tcg2Configuration variable contains the desired TpmDevice type,
  // while PcdTpmInstanceGuid PCD contains the real detected TpmDevice type
  //
  for (Index = 0; Index < sizeof(mTpmInstanceId)/sizeof(mTpmInstanceId[0]); Index++) {
    if (TpmDevice == mTpmInstanceId[Index].TpmDevice) {
      Size = sizeof(mTpmInstanceId[Index].TpmInstanceGuid);
      Status = PcdSetPtrS (PcdTpmInstanceGuid, &Size, &mTpmInstanceId[Index].TpmInstanceGuid);
      ASSERT_EFI_ERROR (Status);
      DEBUG ((EFI_D_INFO, "TpmDevice PCD: %g\n", &mTpmInstanceId[Index].TpmInstanceGuid));
      break;
    }
  }

  //
  // Selection done
  //
  Status = PeiServicesInstallPpi (&gTpmSelectedPpi);
  ASSERT_EFI_ERROR (Status);

  //
  // Even if no TPM is selected or detected, we still need intall TpmInitializationDonePpi.
  // Because TcgPei or Tcg2Pei will not run, but we still need a way to notify other driver.
  // Other driver can know TPM initialization state by TpmInitializedPpi.
  //
  if (CompareGuid (PcdGetPtr(PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceNoneGuid)) {
    Status2 = PeiServicesInstallPpi (&mTpmInitializationDonePpiList);
    ASSERT_EFI_ERROR (Status2);
  }

  return Status;
}
