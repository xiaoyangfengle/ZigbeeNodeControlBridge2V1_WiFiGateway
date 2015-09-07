    .global s_asAPduPool
    .section    .data.s_asAPduPool,"aw",@progbits
    .align 4
    .type    s_asAPduPool, @object
    .size    s_asAPduPool, 36
s_asAPduPool:
    .global pdum_apduZDP
pdum_apduZDP:
    .long    s_asapduZDPInstances
    .short    0
    .short    100
    .short    4
    .zero     2
    .global pdum_apduControl
pdum_apduControl:
    .long    s_asapduControlInstances
    .short    0
    .short    82
    .short    4
    .zero     2
    .global pdum_apdujumbo
pdum_apdujumbo:
    .long    s_asapdujumboInstances
    .short    0
    .short    200
    .short    2
    .zero     2
