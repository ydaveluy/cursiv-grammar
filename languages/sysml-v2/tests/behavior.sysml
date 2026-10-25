// Covers: occurrence/individual usages, connectors (a `connection`-kind
// shortcut, a nary form, `bind a = b`, flow, message), action control
// nodes (accept/send with `via`, assign, terminate, if/else, while, for,
// merge, decide, join, fork), a state machine
// (entry/do/exit/state/transition), requirement extras (actor,
// stakeholder, frame, verify, satisfy with `by`, assert), views (expose
// with a wildcard, render, viewpoint), metadata with "about", a textual
// representation, and a bare block comment.
package Behavior {
    part def Node {
        attribute id : Integer;
    }

    occurrence def Occ;
    individual def Ind;

    port def P {
        attribute v : Real;
    }

    connection def Link;

    part def System {
        part n1 : Node;
        part n2 : Node;
        part n3 : Node;

        connection linkOneTwo connect n1 to n2;
        connection linkAll connect (n1, n2, n3);

        binding bindOneTwo;
        bind n1 = n2;

        flow def DataFlow;
        flow flowOneTwo : DataFlow from n1 to n2;

        message msgOneTwo from n1 to n2;
    }

    action def Handle {
        in item payload : Node;

        accept payload;
        send payload;
        accept payload via payload;
        send payload via payload to payload;
        assign target := payload;
        terminate;

        if true {
            action doThen;
        } else {
            action doElse;
        }

        while true {
            action loopBody;
        }

        for x in payload {
            action forBody;
        }

        merge m1;
        decide d1;
        join j1;
        fork f1;
    }

    state def Machine {
        entry doStartup;
        do doRunning;
        exit doShutdown;

        state Idle;
        state Running;

        transition t1;
    }

    requirement def R1 {
        subject vehicle : Node;
        actor driver : Node;
        stakeholder owner : Node;

        frame concern c1 {
            doc /* the framed concern */
        }

        verify requirement v1 {
            doc /* verification link */
        }
    }

    satisfy requirement r2 by vehicle {
        doc /* satisfaction */
    }

    assert constraint always {
        true
    }

    view def Overview {
        expose Behavior::System::*;
        render Overview::chart;
    }

    viewpoint def StakeholderView;

    rendering def Chart;

    #Base::Tag metadata def Tagged;

    @Base::Tag about System {
        doc /* metadata about the system */
    }

    rep RSpec language "OCL"
    /* system.n1.id > 0 */

    comment
    /* a bare block comment as a package member */
}
