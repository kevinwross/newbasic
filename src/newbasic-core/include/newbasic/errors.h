#pragma once

#include <string>

namespace newbasic {

// GW-BASIC error codes and messages (User's Guide, Appendix A). Only the
// codes reachable in this implementation are raised, but the full table is
// kept so messages and codes match the dialect exactly.
enum ErrorCode : int {
  kErrNextWithoutFor = 1,
  kErrSyntax = 2,
  kErrReturnWithoutGosub = 3,
  kErrOutOfData = 4,
  kErrIllegalFunctionCall = 5,
  kErrOverflow = 6,
  kErrOutOfMemory = 7,
  kErrUndefinedLineNumber = 8,
  kErrSubscriptOutOfRange = 9,
  kErrDuplicateDefinition = 10,
  kErrDivisionByZero = 11,
  kErrIllegalDirect = 12,
  kErrTypeMismatch = 13,
  kErrOutOfStringSpace = 14,
  kErrStringTooLong = 15,
  kErrStringFormulaTooComplex = 16,
  kErrCantContinue = 17,
  kErrUndefinedUserFunction = 18,
  kErrNoResume = 19,
  kErrResumeWithoutError = 20,
  kErrUnprintable = 21,
  kErrMissingOperand = 22,
  kErrLineBufferOverflow = 23,
  kErrDeviceTimeout = 24,
  kErrDeviceFault = 25,
  kErrForWithoutNext = 26,
  kErrOutOfPaper = 27,
  kErrWhileWithoutWend = 29,
  kErrWendWithoutWhile = 30,
  kErrFieldOverflow = 50,
  kErrInternal = 51,
  kErrBadFileNumber = 52,
  kErrFileNotFound = 53,
  kErrBadFileMode = 54,
  kErrFileAlreadyOpen = 55,
  kErrDeviceIOError = 57,
  kErrFileAlreadyExists = 58,
  kErrDiskFull = 61,
  kErrInputPastEnd = 62,
  kErrBadRecordNumber = 63,
  kErrBadFileName = 64,
  kErrDirectStatementInFile = 66,
  kErrTooManyFiles = 67,
  kErrDeviceUnavailable = 68,
  kErrCommunicationBufferOverflow = 69,
  kErrPermissionDenied = 70,
  kErrDiskNotReady = 71,
  kErrDiskMediaError = 72,
  kErrAdvancedFeature = 73,
  kErrRenameAcrossDisks = 74,
  kErrPathFileAccess = 75,
  kErrPathNotFound = 76,
};

// Message text for a code ("Syntax error", "Division by zero", ...).
const char* ErrorMessage(int code);

// GW-BASIC display form: "?<message>" for direct mode,
// "?<message> in <line>" for a program error (line 0 means direct mode).
std::string FormatError(int code, int line);

} // namespace newbasic
