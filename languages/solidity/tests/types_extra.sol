pragma solidity ^0.8.0;

// Constructs not otherwise exercised: function types, a custom-type mapping
// key, `using ... for` with an operator override, address payable arrays,
// a long dotted path, and a file-level constant.

uint256 constant VERSION = 1;

enum Status { Pending, Active, Closed }

library Ops {
    function add(uint256 a, uint256 b) internal pure returns (uint256) {
        return a + b;
    }
}

using Ops for uint256 global;

contract Registry {
    mapping(Status => uint256) counts;
    mapping(address => mapping(uint256 => bool)) seen;

    function(uint256, uint256) internal pure returns (uint256) op;

    address payable[] payees;

    struct Node {
        Registry.Status dummy;
    }

    enum InnerStatus { Ok }

    function configure() external {
        op = Ops.add;
        counts[Status.Active] = 1;
    }

    function total() external view returns (uint256 sum) {
        for (uint256 i = 0; i < payees.length; i = op(i, 1)) {
            sum = op(sum, i);
        }
    }
}
