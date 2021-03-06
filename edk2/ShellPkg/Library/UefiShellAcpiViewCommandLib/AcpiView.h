/** @file
  Header file for AcpiView

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ACPIVIEW_H_
#define ACPIVIEW_H_

/**
  A macro to define the max file name length
**/
#define MAX_FILE_NAME_LEN    128

/**
  Offset to the RSDP revision from the start of the RSDP
**/
#define RSDP_REVISION_OFFSET 15

/**
  Offset to the RSDP length from the start of the RSDP
**/
#define RSDP_LENGTH_OFFSET   20

/**
  The EREPORT_OPTION enum describes ACPI table Reporting options.
**/
typedef enum ReportOption {
  ReportAll,            ///< Report All tables.
  ReportSelected,       ///< Report Selected table.
  ReportTableList,      ///< Report List of tables.
  ReportDumpBinFile,    ///< Dump selected table to a file.
  ReportMax,
} EREPORT_OPTION;

/**
  This function resets the ACPI table error counter to Zero.
**/
VOID
ResetErrorCount (
  VOID
  );

/**
  This function returns the ACPI table error count.

  @retval Returns the count of errors detected in the ACPI tables.
**/
UINT32
GetErrorCount (
  VOID
  );

/**
  This function resets the ACPI table warning counter to Zero.
**/
VOID
ResetWarningCount (
  VOID
  );

/**
  This function returns the ACPI table warning count.

  @retval Returns the count of warning detected in the ACPI tables.
**/
UINT32
GetWarningCount (
  VOID
  );

/**
  This function returns the colour highlighting status.

  @retval TRUE if colour highlighting is enabled.
**/
BOOLEAN
GetColourHighlighting (
  VOID
  );

/**
  This function sets the colour highlighting status.

  @param  Highlight       The Highlight status.

**/
VOID
SetColourHighlighting (
  BOOLEAN Highlight
  );

/**
  This function returns the consistency checking status.

  @retval TRUE if consistency checking is enabled.
**/
BOOLEAN
GetConsistencyChecking (
  VOID
  );

/**
  This function sets the consistency checking status.

  @param  ConsistencyChecking   The consistency checking status.

**/
VOID
SetConsistencyChecking (
  BOOLEAN ConsistencyChecking
  );

/**
  This function returns the ACPI table requirements validation flag.

  @retval TRUE if check for mandatory table presence should be performed.
**/
BOOLEAN
GetMandatoryTableValidate (
  VOID
  );

/**
  This function sets the ACPI table requirements validation flag.

  @param  Validate    Enable/Disable ACPI table requirements validation.
**/
VOID
SetMandatoryTableValidate (
  BOOLEAN Validate
  );

/**
  This function returns the identifier of specification to validate ACPI table
  requirements against.

  @return   ID of specification listing mandatory tables.
**/
UINTN
GetMandatoryTableSpec (
  VOID
  );

/**
  This function sets the identifier of specification to validate ACPI table
  requirements against.

  @param  Spec      ID of specification listing mandatory tables.
**/
VOID
SetMandatoryTableSpec (
  UINTN Spec
  );

/**
  This function processes the table reporting options for the ACPI table.

  @param [in] Signature The ACPI table Signature.
  @param [in] TablePtr  Pointer to the ACPI table data.
  @param [in] Length    The length fo the ACPI table.

  @retval Returns TRUE if the ACPI table should be traced.
**/
BOOLEAN
ProcessTableReportOptions (
  IN CONST UINT32  Signature,
  IN CONST UINT8*  TablePtr,
  IN CONST UINT32  Length
  );

#endif // ACPIVIEW_H_
