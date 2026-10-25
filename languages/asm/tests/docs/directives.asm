; A document of my own: every directive shape besides section/extern/global's
; bare form, which the upstream corpus already exercises.
        bits 64
        use16
        use32
        default rel
        absolute 1000h
        cpu 686
        osabi 0
        org 100h
        static baz
        uppercase
        float daz
        [warning +overflow]
        [map all report]
        global gfun:function default weak
        common cvar 4
        export efun
        safeseh handler
        group grp seg1 seg2
        import kmod func1 func2
        section .foo private data class=CODE overlay=OVL progbits alloc exec write vstart=0 follows=.bar use32 flat align=16 comdat=1:grp tls
