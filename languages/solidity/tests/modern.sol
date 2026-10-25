// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import "./abc.sol" as Abc;

interface IToken {
    function totalSupply() external view returns (uint256);
}

// user-defined value type
type Fixed18 is uint256;

error InsufficientBalance(uint256 available, uint256 required);
error ExternalCallFailed();

library Math {
    function add(uint256 a, uint256 b) internal pure returns (uint256) {
        return a + b;
    }
}

abstract contract Base {
    function name() public virtual returns (string memory);
}

contract Token is IToken, Base {
    using Math for uint256;
    using {addOne} for uint256;

    uint256 public constant DECIMALS = 18;
    uint256 private totalSupply_;
    address payable owner;

    event Transfer(address indexed from, address indexed to, uint256 value);

    modifier onlyOwner() {
        require(msg.sender == address(owner));
        _;
    }

    constructor(uint256 initialSupply) payable {
        totalSupply_ = initialSupply;
        owner = payable(msg.sender);
    }

    function totalSupply() external view override returns (uint256) {
        return totalSupply_;
    }

    function name() public pure override returns (string memory) {
        return unicode"Étalon coin ✓";
    }

    function transfer(address to, uint256 amount) public onlyOwner virtual override(Base) returns (bool) {
        if (amount > totalSupply_) {
            revert InsufficientBalance({available: totalSupply_, required: amount});
        }
        unchecked {
            totalSupply_ -= amount;
        }
        emit Transfer(msg.sender, to, amount);
        return true;
    }

    function slice(uint256[] memory data) public pure returns (uint256[] memory) {
        uint256[] memory copy = data;
        return copy;
    }

    function createAndCall() external returns (bytes memory) {
        Token t = new Token{value: 1 ether}(0);
        (bool ok, bytes memory ret) = address(t).call{value: 0, gas: 21000}("");
        require(ok);
        return ret;
    }

    function tryExternal(address target) external returns (uint256 result) {
        try IToken(target).totalSupply() returns (uint256 v) {
            result = v;
        } catch Error(string memory) {
            revert ExternalCallFailed();
        } catch (bytes memory) {
            result = 0;
        }
    }

    function typeInfo() external pure returns (bytes4) {
        return type(IToken).interfaceId;
    }

    receive() external payable {}

    fallback() external payable {
        revert ExternalCallFailed();
    }

    // soft keywords used as plain identifiers
    function combine(uint256 from, uint256 error, uint256 revert, uint256 global, uint256 transient) external pure returns (uint256) {
        return from + error + revert + global + transient;
    }
}

function addOne(uint256 x) pure returns (uint256) {
    return x + 1;
}

function freeFunction(int256 a, int256 b) pure returns (int256 sum, int256 diff) {
    sum = a + b;
    diff = a - b;
}
