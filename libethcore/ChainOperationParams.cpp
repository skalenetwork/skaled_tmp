/*
    Modifications Copyright (C) 2018-2019 SKALE Labs

    This file is part of cpp-ethereum.

    cpp-ethereum is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    cpp-ethereum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file ChainOperationParams.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "ChainOperationParams.h"

#include <libskale/PushZeroPatch.h>

#include <libdevcore/CommonData.h>
#include <libdevcore/Log.h>

#include <skutils/utils.h>

using namespace std;
using namespace dev;
using namespace eth;

PrecompiledContract::PrecompiledContract( unsigned _base, unsigned _word,
    PrecompiledExecutor const& _exec, u256 const& _startingBlock, h160Set const& _allowedAddresses )
    : PrecompiledContract(
          [=]( bytesConstRef _in, ChainOperationParams const&, u256 const& ) -> bigint {
              bigint s = _in.size();
              bigint b = _base;
              bigint w = _word;
              return b + ( s + 31 ) / 32 * w;
          },
          _exec, _startingBlock, _allowedAddresses ) {}

ChainOperationParams::ChainOperationParams()
    : m_blockReward( "0x4563918244F40000" ),
      minGasLimit( 0x1388 ),
      maxGasLimit( "0x7fffffffffffffff" ),
      gasLimitBoundDivisor( 0x0400 ),
      networkID( 0x0 ),
      minimumDifficulty( 0x020000 ),
      difficultyBoundDivisor( 0x0800 ),
      durationLimit( 0x0d ) {}

EVMSchedule const ChainOperationParams::evmSchedule(
    time_t _lastBlockTimestamp, u256 const& _blockNumber ) const {
    EVMSchedule result;

    // 1 decide by block number
    if ( _blockNumber >= experimentalForkBlock )
        result = ExperimentalSchedule;
    else if ( _blockNumber >= istanbulForkBlock )
        result = IstanbulSchedule;
    else if ( _blockNumber >= constantinopleFixForkBlock )
        result = ConstantinopleFixSchedule;
    else if ( _blockNumber >= constantinopleForkBlock )
        result = ConstantinopleSchedule;
    else if ( _blockNumber >= byzantiumForkBlock )
        result = ByzantiumSchedule;
    else if ( _blockNumber >= EIP158ForkBlock )
        result = EIP158Schedule;
    else if ( _blockNumber >= EIP150ForkBlock )
        result = EIP150Schedule;
    else
        result = HomesteadSchedule;

    // 2 based on previous - decide by timestamp
    if ( PushZeroPatch::isEnabledWhen( *this, _lastBlockTimestamp ) )
        result = PushZeroPatch::makeSchedule( result );

    return result;
}

u256 ChainOperationParams::blockReward( EVMSchedule const& _schedule ) const {
    if ( _schedule.blockRewardOverwrite )
        return *_schedule.blockRewardOverwrite;
    else
        return m_blockReward;
}

void ChainOperationParams::setBlockReward( u256 const& _newBlockReward ) {
    m_blockReward = _newBlockReward;
}

// this will be changed to just extract all **PatchTimestamp params by wildcard,
// thus eliminating if..if
time_t ChainOperationParams::getPatchTimestamp( const std::string& _name ) const {
    if ( _name == "PushZeroPatch" )
        return sChain.pushZeroPatchTimestamp;
    if ( _name == "RevertableFSPatch" )
        return sChain.revertableFSPatchTimestamp;
    if ( _name == "ContractStorageZeroValuePatch" )
        return sChain.contractStorageZeroValuePatchTimestamp;
    assert( false );
}
