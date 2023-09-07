# Switch Config

To configure ports in switch,

```sh
cd /root/bf-sde-9.9.0
ln -s /root/Tofino-QSG/port-setup/ ./
ln -s /root/Tofino-QSG/configure_ports.sh ./
ln -s /root/Tofino-QSG/set_env.sh ./
```

To setup Aurora switch in this configuration, run:
```sh
<SDE> source set_sde.sh
<SDE> ./run_switchd.sh -p switch -r switchd.log
<SDE> ./configure_ports.sh
```

To stop currently running switchd, 
```
<SDE> ps -aux | grep switchd
<SDE> kill -9 <pid>
```

 