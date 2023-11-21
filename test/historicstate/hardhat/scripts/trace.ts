const OWNER_ADDRESS: string = "0x907cd0881E50d359bb9Fd120B1A5A143b1C97De6";
const ZERO_ADDRESS: string = "0xO000000000000000000000000000000000000000";
const INITIAL_MINT: bigint = 10000000000000000000000000000000000000000n;

import {ethers} from "hardhat";

async function waitUntilNextBlock() {

    const current = await hre.ethers.provider.getBlockNumber();
    let newBlock = current;
    console.log(`BLOCK_NUMBER ${current}`);

    while (newBlock == current) {
        newBlock = await hre.ethers.provider.getBlockNumber();
    }

    console.log(`BLOCK_NUMBER ${newBlock}`);

    return current;

}

function CHECK(result: any): void {
    if (!result) {
        const message: string = `Check failed ${result}`
        console.log(message);
        throw message;
    }


}

async function getAndPrintBlockTrace(): Promise<String> {

    let newBlock: number = await hre.ethers.provider.getBlockNumber();

    let blockStr = "0x" + newBlock.toString(16);

    console.log("Got block number" + blockStr);

    const trace = await ethers.provider.send('debug_traceBlockByNumber', [blockStr, {
        "tracer": "allTracer",
        "tracerConfig": {"withLog": true}
    }]);

    console.log(JSON.stringify(trace, null, 4));
    return trace;
}

async function getAndPrintTrace(hash: string): Promise<String> {
//    const trace = await ethers.provider.send('debug_traceTransaction', [hash, {"tracer":"prestateTracer",
//        "tracerConfig": {"diffMode":true}}]);

//    const trace = await ethers.provider.send('debug_traceTransaction', [hash, {"tracer": "callTracer",
//        "tracerConfig": {"withLog":true}}]);

    const trace = await ethers.provider.send('debug_traceTransaction', [hash, {
        "tracer": "allTracer",
        "tracerConfig": {"withLog": true}
    }]);


    console.log(JSON.stringify(trace, null, 4));
    return trace;
}

async function deployWriteAndDestroy(): Promise<void> {

    console.log(`Deploying ...`);

    const factory = await ethers.getContractFactory("Tracer");
    const tracer = await factory.deploy({
        gasLimit: 2100000, // this is just an example value; you'll need to set an appropriate gas limit for your specific function call
    });
    const deployedTracer = await tracer.deployed();


    const deployBn = await ethers.provider.getBlockNumber();

    const hash = deployedTracer.deployTransaction.hash;
    console.log(`Gas limit ${deployedTracer.deployTransaction.gasLimit}`);
    console.log(`Contract deployed to ${deployedTracer.address} at block ${deployBn} tx hash ${hash}`);


    // await waitUntilNextBlock()

    await getAndPrintBlockTrace();
    await getAndPrintTrace(hash)


    console.log(`Now minting`);

    const transferReceipt = await deployedTracer.mint(1000, {
        gasLimit: 2100000, // this is just an example value; you'll need to set an appropriate gas limit for your specific function call
    });
    console.log(`Gas limit ${transferReceipt.gasLimit}`);

    await getAndPrintTrace(transferReceipt.hash);

    /*

    console.log(`Now testing self-destruct`);

    const transferReceipt2 = await lockContract.die("0x690b9a9e9aa1c9db991c7721a92d351db4fac990");
    await transferReceipt2.wait();

    console.log(`Successfully self destructed`);

    console.log(`PASSED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!`);


     */
}

async function main(): Promise<void> {
    await deployWriteAndDestroy();
}

// We recommend this pattern to be able to use async/await everywhere
// and properly handle errors.
main().catch((error: any) => {
    console.error(error);
    process.exitCode = 1;
});
