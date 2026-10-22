#include "newbasic/errors.h"

namespace newbasic {

const char* ErrorMessage(int code) {
  switch (code) {
  case kErrNextWithoutFor:
    return "NEXT without FOR";
  case kErrSyntax:
    return "Syntax error";
  case kErrReturnWithoutGosub:
    return "RETURN without GOSUB";
  case kErrOutOfData:
    return "Out of DATA";
  case kErrIllegalFunctionCall:
    return "Illegal function call";
  case kErrOverflow:
    return "Overflow";
  case kErrOutOfMemory:
    return "Out of memory";
  case kErrUndefinedLineNumber:
    return "Undefined line number";
  case kErrSubscriptOutOfRange:
    return "Subscript out of range";
  case kErrDuplicateDefinition:
    return "Duplicate Definition";
  case kErrDivisionByZero:
    return "Division by zero";
  case kErrIllegalDirect:
    return "Illegal direct";
  case kErrTypeMismatch:
    return "Type mismatch";
  case kErrOutOfStringSpace:
    return "Out of string space";
  case kErrStringTooLong:
    return "String too long";
  case kErrStringFormulaTooComplex:
    return "String formula too complex";
  case kErrCantContinue:
    return "Can't continue";
  case kErrUndefinedUserFunction:
    return "Undefined user function";
  case kErrNoResume:
    return "No RESUME";
  case kErrResumeWithoutError:
    return "RESUME without error";
  case kErrMissingOperand:
    return "Missing operand";
  case kErrLineBufferOverflow:
    return "Line buffer overflow";
  case kErrDeviceTimeout:
    return "Device Timeout";
  case kErrDeviceFault:
    return "Device Fault";
  case kErrForWithoutNext:
    return "FOR Without NEXT";
  case kErrOutOfPaper:
    return "Out of Paper";
  case kErrWhileWithoutWend:
    return "WHILE without WEND";
  case kErrWendWithoutWhile:
    return "WEND without WHILE";
  case kErrFieldOverflow:
    return "FIELD overflow";
  case kErrInternal:
    return "Internal error";
  case kErrBadFileNumber:
    return "Bad file number";
  case kErrFileNotFound:
    return "File not found";
  case kErrBadFileMode:
    return "Bad file mode";
  case kErrFileAlreadyOpen:
    return "File already open";
  case kErrDeviceIOError:
    return "Device I/O Error";
  case kErrFileAlreadyExists:
    return "File already exists";
  case kErrDiskFull:
    return "Disk full";
  case kErrInputPastEnd:
    return "Input past end";
  case kErrBadRecordNumber:
    return "Bad record number";
  case kErrBadFileName:
    return "Bad file name";
  case kErrDirectStatementInFile:
    return "Direct statement in file";
  case kErrTooManyFiles:
    return "Too many files";
  case kErrDeviceUnavailable:
    return "Device Unavailable";
  case kErrCommunicationBufferOverflow:
    return "Communication buffer overflow";
  case kErrPermissionDenied:
    return "Permission Denied";
  case kErrDiskNotReady:
    return "Disk not Ready";
  case kErrDiskMediaError:
    return "Disk media error";
  case kErrAdvancedFeature:
    return "Advanced Feature";
  case kErrRenameAcrossDisks:
    return "Rename across disks";
  case kErrPathFileAccess:
    return "Path/File access error";
  case kErrPathNotFound:
    return "Path not found";
  default:
    return "Unprintable error";
  }
}

std::string FormatError(int code, int line, const std::string& detail) {
  std::string text = "?";
  text += ErrorMessage(code);
  if (line > 0) {
    text += " in ";
    text += std::to_string(line);
  }
  if (!detail.empty() && detail != ErrorMessage(code)) {
    text += ": ";
    text += detail;
  }
  return text;
}

} // namespace newbasic
