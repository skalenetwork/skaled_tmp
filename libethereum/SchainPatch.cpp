#include "SchainPatch.h"

#include <libethcore/ChainOperationParams.h>

using namespace dev::eth;

ChainOperationParams SchainPatch::chainParams;
std::atomic< time_t > SchainPatch::committedBlockTimestamp;

SchainPatchEnum getEnumForPatchName( const std::string& _patchName ) {
    if ( _patchName == "RevertableFSPatch" )
        return SchainPatchEnum::RevertableFSPatch;
    else if ( _patchName == "PrecompiledConfigPatch" )
        return SchainPatchEnum::PrecompiledConfigPatch;
    else if ( _patchName == "PowCheckPatch" )
        return SchainPatchEnum::PowCheckPatch;
    else if ( _patchName == "ContractStorageZeroValuePatch" )
        return SchainPatchEnum::ContractStorageZeroValuePatch;
    else if ( _patchName == "PushZeroPatch" )
        return SchainPatchEnum::PushZeroPatch;
    else if ( _patchName == "SkipInvalidTransactionsPatch" )
        return SchainPatchEnum::SkipInvalidTransactionsPatch;
    else
        throw std::out_of_range( _patchName );
}

std::string getPatchNameForEnum( SchainPatchEnum enumValue ) {
    switch ( enumValue ) {
    case SchainPatchEnum::RevertableFSPatch:
        return "RevertableFSPatch";
    case SchainPatchEnum::PrecompiledConfigPatch:
        return "PrecompiledConfigPatch";
    case SchainPatchEnum::PowCheckPatch:
        return "PowCheckPatch";
    case SchainPatchEnum::ContractStorageZeroValuePatch:
        return "ContractStorageZeroValuePatch";
    case SchainPatchEnum::PushZeroPatch:
        return "PushZeroPatch";
    case SchainPatchEnum::SkipInvalidTransactionsPatch:
        return "SkipInvalidTransactionsPatch";
    default:
        throw std::out_of_range( "UnknownPatch" );
    }
}

void SchainPatch::init( const dev::eth::ChainOperationParams& _cp ) {
    chainParams = _cp;
    for ( size_t i = 0; i < _cp.sChain._patchTimestamps.size(); ++i ) {
        printInfo( getPatchNameForEnum( static_cast< SchainPatchEnum >( i ) ),
            _cp.sChain._patchTimestamps[i] );
    }
}

void SchainPatch::useLatestBlockTimestamp( time_t _timestamp ) {
    committedBlockTimestamp = _timestamp;
}

void SchainPatch::printInfo( const std::string& _patchName, time_t _timeStamp ) {
    if ( _timeStamp == 0 ) {
        cnote << "Patch " << _patchName << " is disabled";
    } else {
        cnote << "Patch " << _patchName << " is set at timestamp " << _timeStamp;
    }
}
bool SchainPatch::isPatchEnabled(
    SchainPatchEnum _patchEnum, const dev::eth::BlockChain& _bc, dev::eth::BlockNumber _bn ) {
    time_t timestamp = chainParams.getPatchTimestamp( _patchEnum );
    return _bc.isPatchTimestampActiveInBlockNumber( timestamp, _bn );
}
bool SchainPatch::isPatchEnabledWhen(
    SchainPatchEnum _patchEnum, time_t _committedBlockTimestamp ) {
    time_t activationTimestamp = chainParams.getPatchTimestamp( _patchEnum );
    return activationTimestamp != 0 && _committedBlockTimestamp >= activationTimestamp;
}

EVMSchedule PushZeroPatch::makeSchedule( const EVMSchedule& _base ) {
    EVMSchedule ret = _base;
    ret.havePush0 = true;
    return ret;
}
