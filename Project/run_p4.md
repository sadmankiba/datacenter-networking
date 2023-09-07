# Run P4 Programs with BF SDE

## Software versions

For writing this guide, BF SDE v9.9.0 is used. For running Tofino model, BF SDE was installed in a server with Ubuntu 20.04 LTS OS. 

## Running a P4 program

Prerequisites:
- SDE installed. 
- `SDE` and `SDE_INSTALL` environment variables have been set.

```sh
<SDE> $ export SDE=`pwd`
<SDE> $ export SDE_INSTALL=$SDE/install
```

- Tofino kernel mode driver has been loaded.
```
<SDE> $ cd install/bin
$ ./bf_kdrv_mod_load $SDE_INSTALL
```

- The P4 program has been compiled and all its artifacts are available in SDE home directory. The example programs are built when installing SDE.

### On Tofino hardware

All prerequisites necessary. Now, run the `bf_switchd` application, passing it the name of the P4 program that defines your switch logic.

```sh
<SDE> $ cd $SDE
<SDE> $ ./run_switchd.sh -p <p4_name> # e.g. p4_name = switch
```
### On Tofino model

All prerequisites except third, loading driver, is necessary. 

```sh
<SDE> $ ./run_tofino_model.sh --arch tofino -p <p4_name> # e.g. p4_name = switch
<SDE> $ ./run_switchd.sh --arch tofino -p <p4_name>
```

Ref: [1], sec 3.6.

### Testing

After running `bf_switchd` with an example P4 program, it can be tested with PTF-
```
<SDE> $ ./run_p4_tests.sh -p <p4_name>
```

## Compiling a P4 program

`bf-p4c` can compile a P4 program that is written for TNA or V1 model. 

To compile a P4_16 program written for V1model, use [2]:
```
bf-p4c --std p4-16 --arch v1model --p4runtime-files <prog>.p4info.pb.txt <prog>.p4
```

The `bf-p4c` compiler generates the following artifacts: 

```sh
.
├── basic.p4info.pb.txt
└── basic.tofino
    ├── basic.conf  # Contains path of the other artifacts
    ├── manifest.json
    └── pipe
        ├── basic.bfa
        ├── context.json  # Low level assembly info for the driver to interpret the program into HW
        ├── graphs
        │   └── 0_table_flow_graph_ingress.dot
        ├── logs
        └── tofino.bin    # Low level assembly info for the driver to interpret the program into HW
```

Building a P4 program with `p4studio` allows to compile with high-level commands and use `bfshell` with that program. To build your p4 program with `p4studio`,
- Add your program in `$SDE/p4studio/pkgsrc/p4-examples/programs/` directory. 
- Add your program name in `$SDE/pkgsrc/p4-examples/CMakeLists.txt`-
```
set (P4_14_Programs
  ...
  <program_name>
```

- Then, run 
```
$ p4studio build p4-14-programs
$ ./run_switchd.sh -p <your_program>
```
The build command will create 
- configuration files for this p4 program in `$SDE/install/share/p4/targets/tofino*/` directories. 
- a directory in `$SDE/build/pkgsrc/p4-examples/` for intermediate build files.
- a directory in `$SDE/install/lib/tofinopd` for object files such as `libpd.so`. 
- a directory in `$SDE/install/share/tofinopd` for some JSON configuration files.

## Testing a P4 program
[Needs to be updated]


## Reference 
[1] Barefoot Wedge 100B Switch Platform User Guide, March 2018.
[2] P4Runtime API Guide, Jul 2019. Intel Research and Documentation center.