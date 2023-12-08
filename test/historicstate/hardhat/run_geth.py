import subprocess
import time
from web3 import Web3, HTTPProvider

def run_geth_container():
    # Pull the Docker image
    subprocess.run(["docker", "pull", "ethereum/client-go"])

    # Run the Geth container in detached mode
    subprocess.run([
        "docker", "run", "-d", "--name", "ethereum-node",
        "-p", "8545:8545", "-p", "30303:30303",
        "ethereum/client-go", "--dev", "--http",
        "--http.addr", "0.0.0.0", "--http.corsdomain", "*",
        "--http.api", "personal,eth,net,web3,debug"
    ])

def add_ether_to_account(address, amount):
    # Connect to the Geth node
    web3 = Web3(HTTPProvider("http://localhost:8545"))

    # Check if the connection is successful
    if not web3.isConnected():
        print("Failed to connect to the Ethereum node.")
        return

    # Unlock the default account (coinbase)
    coinbase = web3.eth.coinbase
    web3.geth.personal.unlockAccount(coinbase, '', 0)

    # Convert Ether to Wei
    value = web3.toWei(amount, 'ether')

    # Create and send the transaction
    tx_hash = web3.eth.sendTransaction({'from': coinbase, 'to': address, 'value': value})

    # Wait for the transaction to be mined
    web3.eth.waitForTransactionReceipt(tx_hash)

    print(f"Successfully sent {amount} ETH to {address}")

# Main execution
if __name__ == "__main__":
    try:
        run_geth_container()
        # Wait a bit for the node to be fully up and running
        time.sleep(10)
        add_ether_to_account("0x907cd0881E50d359bb9Fd120B1A5A143b1C97De6", 1000)
    except Exception as e:
        print(f"An error occurred: {e}")
