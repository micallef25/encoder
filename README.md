# encoder
Take two on accelerating an encoder pipeline

# todo

- [ ] organize test benches
- [ ] organize ifdefs in test benches
- [ ] stream CDC
- [ ] test bit packing pack data into 128 bit wide dma
- [ ] optimiza SHA padding currently it wriets a byte every 4 cycles. This can be optimized to 2 cycles
- [ ] padding can be optmizied further if cdc writes 16 bits to a stream instead of 8
- [ ] test CDC
- [ ] bench mark sha and cdc combined

# Hardware functions available

## axi_wide

a hardware function for testing packing data in

## axi_normal

a hardware function for compairson of runtime to packing data in

## sha 

input is a buffer output is one SHA key

## cdc to sha

input is a buffer output is a multitude of keys based off of chunks found
