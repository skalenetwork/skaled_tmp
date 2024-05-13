#ifndef SCHAINPATCHENUM_H
#define SCHAINPATCHENUM_H

#include <stdexcept>
#include <string>

enum class SchainPatchEnum {
    RevertableFSPatch,
    PrecompiledConfigPatch,
    PowCheckPatch,
    CorrectForkInPowPatch,
    ContractStorageZeroValuePatch,
    PushZeroPatch,
    VerifyDaSigsPatch,
    ContractStoragePatch,
    StorageDestructionPatch,
    SkipInvalidTransactionsPatch,
    SelfdestructStorageLimitPatch,
    EIP1559TransactionsPatch,
    VerifyBlsSyncPatch,
    PatchesCount
};

extern SchainPatchEnum getEnumForPatchName( const std::string& _patchName );
extern std::string getPatchNameForEnum( SchainPatchEnum enumValue );


#endif  // SCHAINPATCHENUM_H
