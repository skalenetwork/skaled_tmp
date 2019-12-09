/*
    Copyright (C) 2019-present, SKALE Labs

    This file is part of skaled.

    skaled is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    skaled is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with skaled.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * @file SnapshotHashAgent.cpp
 * @author Oleh Nikolaiev
 * @date 2019
 */

#include "SnapshotHashAgent.h"

#include <libconsensus/libBLS/bls/bls.h>
#include <libethcore/CommonJS.h>
#include <libweb3jsonrpc/Skale.h>
#include <skutils/rest_call.h>

std::pair< dev::h256, libff::alt_bn128_G1 > SnapshotHashAgent::voteForHash() {
    std::map< dev::h256, size_t > map_hash;

    const std::lock_guard< std::mutex > lock( this->hashes_mutex );

    for ( size_t i = 0; i < this->n_; ++i ) {
        if ( this->chain_params_.nodeInfo.id == this->chain_params_.sChain.nodes[i].id ) {
            continue;
        }

        map_hash[this->hashes_[i]] += 1;
    }

    auto it = std::find_if( map_hash.begin(), map_hash.end(),
        [this]( const std::pair< dev::h256, size_t > p ) { return 3 * p.second > 2 * this->n_; } );

    if ( it == map_hash.end() ) {
        throw std::logic_error( "note enough votes to choose hash" );
    } else {
        size_t t = ( 2 * this->n_ + 2 ) / 3;
        signatures::Bls bls_instanse = signatures::Bls( t, this->n_ );
        std::vector< size_t > idx;
        std::vector< libff::alt_bn128_G1 > signatures;
        for ( size_t i = 0; i < this->n_; ++i ) {
            if ( this->chain_params_.nodeInfo.id == this->chain_params_.sChain.nodes[i].id ) {
                continue;
            }

            if ( this->hashes_[i] == ( *it ).first ) {
                this->nodes_to_download_snapshot_from_.push_back( i );
                idx.push_back( i );
                signatures.push_back( this->signatures_[i] );
            }
        }
        std::vector< libff::alt_bn128_Fr > lagrange_coeffs = bls_instanse.LagrangeCoeffs( idx );
        libff::alt_bn128_G1 common_signature =
            bls_instanse.SignatureRecover( signatures, lagrange_coeffs );

        libff::alt_bn128_G2 common_public;

        std::string original_json = this->chain_params_.getOriginalJson();
        nlohmann::json joConfig = nlohmann::json::parse( original_json );
        common_public.X.c0 = libff::alt_bn128_Fq(
            joConfig["skaleConfig"]["nodeInfo"]["wallets"]["ima"]["insecureCommonBLSPublicKey0"]
                .get< std::string >()
                .c_str() );
        common_public.X.c1 = libff::alt_bn128_Fq(
            joConfig["skaleConfig"]["nodeInfo"]["wallets"]["ima"]["insecureCommonBLSPublicKey1"]
                .get< std::string >()
                .c_str() );
        common_public.Y.c0 = libff::alt_bn128_Fq(
            joConfig["skaleConfig"]["nodeInfo"]["wallets"]["ima"]["insecureCommonBLSPublicKey2"]
                .get< std::string >()
                .c_str() );
        common_public.Y.c1 = libff::alt_bn128_Fq(
            joConfig["skaleConfig"]["nodeInfo"]["wallets"]["ima"]["insecureCommonBLSPublicKey3"]
                .get< std::string >()
                .c_str() );
        common_public.Z = libff::alt_bn128_Fq2::one();

        try {
            if ( !bls_instanse.Verification(
                     std::make_shared< std::array< uint8_t, 32 > >( ( *it ).first.asArray() ),
                     common_signature, common_public ) ) {
                throw std::logic_error( " Common signature was not verified during voteForHash " );
            }
        } catch ( std::exception& ex ) {
            std::cerr << cc::error(
                             "Exception while collecting snapshot hash from other skaleds: " )
                      << cc::warn( ex.what() ) << "\n";
        }

        return std::make_pair( ( *it ).first, common_signature );
    }
}

std::vector< std::string > SnapshotHashAgent::getNodesToDownloadSnapshotFrom(
    unsigned block_number ) {
    std::vector< std::thread > threads;
    for ( size_t i = 0; i < this->n_; ++i ) {
        if ( this->chain_params_.nodeInfo.id == this->chain_params_.sChain.nodes[i].id ) {
            continue;
        }

        threads.push_back( std::thread( [this, i, block_number]() {
            try {
                nlohmann::json joCall = nlohmann::json::object();
                joCall["jsonrpc"] = "2.0";
                joCall["method"] = "skale_getSnapshotSignature";
                nlohmann::json obj = {block_number};
                joCall["params"] = obj;
                skutils::rest::client cli;
                bool fl = cli.open(
                    "http://" + this->chain_params_.sChain.nodes[i].ip + ':' +
                    ( this->chain_params_.sChain.nodes[i].port + 3 ).convert_to< std::string >() );
                if ( !fl ) {
                    std::cerr << cc::fatal( "FATAL:" )
                              << cc::error(
                                     " Exception while trying to connect to another skaled: " )
                              << cc::warn( "connection refused" ) << "\n";
                }
                skutils::rest::data_t d = cli.call( joCall );
                if ( d.empty() ) {
                    throw std::runtime_error(
                        "Sgx Server call to skale_getSnapshotSignature failed" );
                }
                nlohmann::json joResponse = nlohmann::json::parse( d.s_ )["result"];
                std::string str_hash = joResponse["hash"];
                libff::alt_bn128_G1 signature = libff::alt_bn128_G1(
                    libff::alt_bn128_Fq( joResponse["X"].get< std::string >().c_str() ),
                    libff::alt_bn128_Fq( joResponse["Y"].get< std::string >().c_str() ),
                    libff::alt_bn128_Fq::one() );

                const std::lock_guard< std::mutex > lock( this->hashes_mutex );

                this->hashes_[i] = dev::h256( str_hash );
                this->signatures_[i] = signature;
            } catch ( std::exception& ex ) {
                std::cerr << cc::error(
                                 "Exception while collecting snapshot hash from other skaleds: " )
                          << cc::warn( ex.what() ) << "\n";
            }
        } ) );
    }

    for ( auto& thr : threads ) {
        thr.join();
    }

    this->voted_hash_ = this->voteForHash();

    std::vector< std::string > ret;
    for ( const size_t idx : this->nodes_to_download_snapshot_from_ ) {
        std::string ret_value =
            std::string( "http://" ) + std::string( this->chain_params_.sChain.nodes[idx].ip ) +
            std::string( ":" ) +
            ( this->chain_params_.sChain.nodes[idx].port + 3 ).convert_to< std::string >();
        ret.push_back( ret_value );
    }

    return ret;
}

std::pair< dev::h256, libff::alt_bn128_G1 > SnapshotHashAgent::getVotedHash() const {
    assert( this->voted_hash_.first != dev::h256() );
    return this->voted_hash_;
}
