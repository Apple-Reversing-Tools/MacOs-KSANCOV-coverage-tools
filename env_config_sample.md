kern.kcov.od.logging_enabled: 0 -> 1
foo@fooui-gasang-keompyuteo ~ % ls    
foo@fooui-gasang-keompyuteo ~ %  sysctl kern.kcov.od.config               
kern.kcov.od.config: 16
foo@fooui-gasang-keompyuteo ~ %  sysctl kern.kcov.od.logging_enabled
kern.kcov.od.logging_enabled: 1
foo@fooui-gasang-keompyuteo ~ %  sysctl kern.kcov.od.support_enabled 
kern.kcov.od.support_enabled: 1




foo@fooui-gasang-keompyuteo ~ % ls /dev/ | grep ksancov
ksancov
foo@fooui-gasang-keompyuteo ~ % ls /dev/ | grep klog   
klog
foo@fooui-gasang-keompyuteo ~ % 




foo@fooui-gasang-keompyuteo ~ % nm -m /Library/Developer/KDKs/KDK_15.6_24G84.kdk/System/Library/Kernels/kernel.kasan.vmapple | grep ksancov
fffffe000965bcf8 (__DATA,__init) non-external ___startup_lck_grp_spec_ksancov_lck_grp
fffffe000965bd60 (__DATA,__init) non-external ___startup_lck_mtx_spec_ksancov_od_lck
fffffe000965bd48 (__DATA,__init) non-external ___startup_lck_rw_spec_ksancov_devs_lck
fffffe000717f855 (__TEXT,__os_log) non-external _kcov_ksancov_bookmark_on_demand_module._os_log_fmt
fffffe000717f87e (__TEXT,__os_log) non-external _kcov_ksancov_bookmark_on_demand_module._os_log_fmt.25
fffffe000717f8b5 (__TEXT,__os_log) non-external _kcov_ksancov_bookmark_on_demand_module._os_log_fmt.28
fffffe000717f8f2 (__TEXT,__os_log) non-external _kcov_ksancov_bookmark_on_demand_module._os_log_fmt.29
fffffe000717f91b (__TEXT,__os_log) non-external _kcov_ksancov_bookmark_on_demand_module._os_log_fmt.30
fffffe0007346458 (__DATA_CONST,__kalloc_var) non-external _kcov_ksancov_bookmark_on_demand_module.kalloc_type_view_453
fffffe00073464a8 (__DATA_CONST,__kalloc_var) non-external _kcov_ksancov_bookmark_on_demand_module.kalloc_type_view_458
fffffe00095aca5c (__TEXT_EXEC,__text) external _kcov_ksancov_init_thread
fffffe00095acee4 (__TEXT_EXEC,__text) external _kcov_ksancov_pcs_init
fffffe00095aca64 (__TEXT_EXEC,__text) external _kcov_ksancov_trace_guard
fffffe00095dd808 (__TEXT_EXEC,__text) non-external _kcov_ksancov_trace_guard.cold.1
fffffe00095acacc (__TEXT_EXEC,__text) external _kcov_ksancov_trace_pc
fffffe00095dd898 (__TEXT_EXEC,__text) non-external _kcov_ksancov_trace_pc.cold.1
fffffe00095acb34 (__TEXT_EXEC,__text) external _kcov_ksancov_trace_pc_guard_init
fffffe00072c0f58 (__DATA_CONST,__const) non-external _ksancov_cdev
fffffe00095ad0f4 (__TEXT_EXEC,__text) non-external _ksancov_close
fffffe00073396d0 (__DATA_CONST,__assert) non-external _ksancov_counters_alloc.__desc
fffffe00095ad954 (__TEXT_EXEC,__text) non-external _ksancov_detach
fffffe0007339694 (__DATA_CONST,__assert) non-external _ksancov_detach.__desc
fffffe00073396a8 (__DATA_CONST,__assert) non-external _ksancov_detach.__desc.40
fffffe00095dd95c (__TEXT_EXEC,__text) non-external _ksancov_detach.cold.1
fffffe00095dd968 (__TEXT_EXEC,__text) non-external _ksancov_detach.cold.2
fffffe00095acf74 (__TEXT_EXEC,__text) non-external _ksancov_dev_clone
fffffe00097ac1d0 (__DATA,__bss) non-external _ksancov_devs
fffffe00097ac198 (__DATA,__bss) non-external _ksancov_devs_lck
fffffe00095ada0c (__TEXT_EXEC,__text) non-external _ksancov_do_map
fffffe00097ac1c8 (__DATA,__bss) non-external _ksancov_edgemap
fffffe00097ac3d0 (__DATA,__bss) non-external _ksancov_enabled
fffffe000717f960 (__TEXT,__os_log) non-external _ksancov_handle_on_demand_cmd._os_log_fmt
fffffe000717f985 (__TEXT,__os_log) non-external _ksancov_handle_on_demand_cmd._os_log_fmt.44
fffffe000717f9ab (__TEXT,__os_log) non-external _ksancov_handle_on_demand_cmd._os_log_fmt.45
fffffe00095dd708 (__TEXT_EXEC,__text) non-external (was a private external) _ksancov_init
fffffe00073463b8 (__DATA_CONST,__kalloc_var) non-external _ksancov_init.kalloc_type_view_184
fffffe0007346408 (__DATA_CONST,__kalloc_var) non-external _ksancov_init.kalloc_type_view_186
fffffe00095acee8 (__TEXT_EXEC,__text) external _ksancov_init_dev
fffffe00095ad1c8 (__TEXT_EXEC,__text) non-external _ksancov_ioctl
fffffe00095dd918 (__TEXT_EXEC,__text) non-external _ksancov_ioctl.cold.1
fffffe00095dd944 (__TEXT_EXEC,__text) non-external _ksancov_ioctl.cold.2
fffffe00095dd950 (__TEXT_EXEC,__text) non-external _ksancov_ioctl.cold.3
fffffe0009687f58 (__DATA,__lock_grp) non-external _ksancov_lck_grp
fffffe00097ac1c4 (__DATA,__bss) non-external _ksancov_od_enabled_count
fffffe00097ac1a8 (__DATA,__bss) non-external _ksancov_od_lck
fffffe00097ac1c0 (__DATA,__bss) non-external _ksancov_od_modules_count
fffffe00095acfbc (__TEXT_EXEC,__text) non-external _ksancov_open
fffffe00073396bc (__DATA_CONST,__assert) non-external _ksancov_trace_alloc.__desc
foo@fooui-gasang-keompyuteo ~ % 

