/**@file
  Produce a default memory type information HOB unless we can determine, from
  the existence of the "MemoryTypeInformation" variable, that the DXE IPL PEIM
  will produce the HOB.

  Copyright (C) 2017, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Guid/MemoryTypeInformation.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Uefi/UefiMultiPhase.h>

#include "Platform.h"

STATIC EFI_MEMORY_TYPE_INFORMATION mDefaultMemoryTypeInformation[] = {
  { EfiACPIMemoryNVS,       0x004 },
  { EfiACPIReclaimMemory,   0x008 },
  { EfiReservedMemoryType,  0x004 },
  { EfiRuntimeServicesData, 0x024 },
  { EfiRuntimeServicesCode, 0x030 },
  { EfiBootServicesCode,    0x180 },
  { EfiBootServicesData,    0xF00 },
  { EfiMaxMemoryType,       0x000 }
};

STATIC
VOID
BuildMemTypeInfoHob (
  VOID
  )
{
  BuildGuidDataHob (
    &gEfiMemoryTypeInformationGuid,
    mDefaultMemoryTypeInformation,
    sizeof mDefaultMemoryTypeInformation
    );
  DEBUG ((
    DEBUG_INFO,
    "%a: default memory type information HOB built\n",
    __FUNCTION__
    ));
}

/**
  Notification function called when EFI_PEI_READ_ONLY_VARIABLE2_PPI becomes
  available.

  @param[in] PeiServices      Indirect reference to the PEI Services Table.
  @param[in] NotifyDescriptor Address of the notification descriptor data
                              structure.
  @param[in] Ppi              Address of the PPI that was installed.

  @return  Status of the notification. The status code returned from this
           function is ignored.
**/
STATIC
EFI_STATUS
EFIAPI
OnReadOnlyVariable2Available (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_PEI_READ_ONLY_VARIABLE2_PPI *ReadOnlyVariable2;
  UINTN                           DataSize;
  EFI_STATUS                      Status;

  DEBUG ((DEBUG_VERBOSE, "%a: %a\n", gEfiCallerBaseName, __FUNCTION__));

  //
  // Check if the "MemoryTypeInformation" variable exists, in the
  // gEfiMemoryTypeInformationGuid namespace.
  //
  ReadOnlyVariable2 = Ppi;
  DataSize = 0;
  Status = ReadOnlyVariable2->GetVariable (
                                ReadOnlyVariable2,
                                EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
                                &gEfiMemoryTypeInformationGuid,
                                NULL,
                                &DataSize,
                                NULL
                                );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // The variable exists; the DXE IPL PEIM will build the HOB from it.
    //
    return EFI_SUCCESS;
  }
  //
  // Install the default memory type information HOB.
  //
  BuildMemTypeInfoHob ();
  return EFI_SUCCESS;
}

//
// Notification object for registering the callback, for when
// EFI_PEI_READ_ONLY_VARIABLE2_PPI becomes available.
//
STATIC CONST EFI_PEI_NOTIFY_DESCRIPTOR mReadOnlyVariable2Notify = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH |
   EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),  // Flags
  &gEfiPeiReadOnlyVariable2PpiGuid,         // Guid
  OnReadOnlyVariable2Available              // Notify
};

VOID
MemTypeInfoInitialization (
  VOID
  )
{
  EFI_STATUS Status;

  if (!FeaturePcdGet (PcdSmmSmramRequire) &&
      FeaturePcdGet (PcdMemVarstoreEmuEnable)) {
    //
    // EFI_PEI_READ_ONLY_VARIABLE2_PPI will never be available; install
    // the default memory type information HOB right away.
    //
    BuildMemTypeInfoHob ();
    return;
  }

  Status = PeiServicesNotifyPpi (&mReadOnlyVariable2Notify);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: failed to set up R/O Variable 2 callback: %r\n",
      __FUNCTION__,
      Status
      ));
    //
    // Install the default HOB as a last resort.
    //
    BuildMemTypeInfoHob ();
  }
}
