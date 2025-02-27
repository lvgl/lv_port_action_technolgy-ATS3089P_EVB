# originates from common-rom.ld

zephyr_linker_section(NAME init KVMA RAM_REGION GROUP RODATA_REGION)
zephyr_linker_section_obj_level(SECTION init LEVEL PRE_KERNEL_1)
zephyr_linker_section_obj_level(SECTION init LEVEL PRE_KERNEL_2)
zephyr_linker_section_obj_level(SECTION init LEVEL POST_KERNEL)
zephyr_linker_section_obj_level(SECTION init LEVEL APPLICATION)
zephyr_linker_section_obj_level(SECTION init LEVEL SMP)

zephyr_linker_section(NAME device KVMA RAM_REGION GROUP RODATA_REGION)
zephyr_linker_section_obj_level(SECTION device LEVEL PRE_KERNEL_1)
zephyr_linker_section_obj_level(SECTION device LEVEL PRE_KERNEL_2)
zephyr_linker_section_obj_level(SECTION device LEVEL POST_KERNEL)
zephyr_linker_section_obj_level(SECTION device LEVEL APPLICATION)
zephyr_linker_section_obj_level(SECTION device LEVEL SMP)

if(CONFIG_GEN_SW_ISR_TABLE AND NOT CONFIG_DYNAMIC_INTERRUPTS)
  # ld align has been changed to subalign to provide identical behavior scatter vs. ld.
  zephyr_linker_section(NAME sw_isr_table KVMA FLASH GROUP RODATA_REGION SUBALIGN ${CONFIG_ARCH_SW_ISR_TABLE_ALIGN} NOINPUT)
  zephyr_linker_section_configure(
    SECTION sw_isr_table
    INPUT ".gnu.linkonce.sw_isr_table*"
  )
endif()

zephyr_linker_section(NAME initlevel_error KVMA RAM_REGION GROUP RODATA_REGION NOINPUT)
zephyr_linker_section_configure(SECTION initlevel_error INPUT ".z_init_[_A-Z0-9]*" KEEP SORT NAME)
# How to do cross linker ?
# ASSERT(SIZEOF(initlevel_error) == 0, "Undefined initialization levels used.")


if(CONFIG_CPLUSPLUS)
  zephyr_linker_section(NAME ctors KVMA RAM_REGION GROUP RODATA_REGION NOINPUT)
  #
  # The compiler fills the constructor pointers table below,
  # hence symbol __CTOR_LIST__ must be aligned on word
  # boundary.  To align with the C++ standard, the first elment
  # of the array contains the number of actual constructors. The
  # last element is NULL.
  #
# ToDo: Checkup on scatter loading. How to manage ?
# https://www.keil.com/support/man/docs/armlink/armlink_pge1362066006368.htm
# https://developer.arm.com/documentation/dui0378/g/The-ARM-C-and-C---Libraries
#  if(CONFIG_64BIT)
#		. = ALIGN(8);
#		__CTOR_LIST__ = .;
#		QUAD((__CTOR_END__ - __CTOR_LIST__) / 8 - 2)
#		KEEP(*(SORT_BY_NAME(".ctors*")))
#		QUAD(0)
#		__CTOR_END__ = .;
#  else()
#		. = ALIGN(4);
#		__CTOR_LIST__ = .;
#		LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
#		KEEP(*(SORT_BY_NAME(".ctors*")))
#		LONG(0)
#		__CTOR_END__ = .;
#  endif()
#	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#
#	SECTION_PROLOGUE(init_array,,)
#	{
#		. = ALIGN(4);
#		__init_array_start = .;
#		KEEP(*(SORT_BY_NAME(".init_array*")))
#		__init_array_end = .;
#	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
endif()

if(CONFIG_USERSPACE)
  # Build-time assignment of permissions to kernel objects to
  # threads declared with K_THREAD_DEFINE()
  zephyr_linker_section(
    NAME z_object_assignment_area
    VMA FLASH NOINPUT
    SUBALIGN 4
  )
  zephyr_linker_section_configure(
    SECTION z_object_assignment
    INPUT ".z_object_assignment.static.*"
    KEEP SORT NAME
  )
endif()

zephyr_linker_section(
  NAME app_shmem_regions
  KVMA RAM_REGION GROUP RODATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT}
)
zephyr_linker_section_configure(
  SECTION app_shmem_regions
  INPUT ".app_regions.*"
  KEEP SORT NAME
)

if(CONFIG_NET_SOCKETS)
  zephyr_iterable_section(NAME net_socket_register KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
endif()


if(CONFIG_NET_L2_PPP)
  zephyr_iterable_section(NAME ppp_protocol_handler KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
endif()

zephyr_iterable_section(NAME bt_l2cap_fixed_chan KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)

if(CONFIG_BT_BREDR)
  zephyr_iterable_section(NAME bt_l2cap_br_fixed_chan KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
endif()

if(CONFIG_BT_CONN)
  zephyr_iterable_section(NAME bt_conn_cb KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
endif()

zephyr_iterable_section(NAME bt_gatt_service_static KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)

if(CONFIG_BT_MESH)
  zephyr_iterable_section(NAME bt_mesh_subnet_cb KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
  zephyr_iterable_section(NAME bt_mesh_app_key_cb KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)

  zephyr_iterable_section(NAME bt_mesh_hb_cb KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
endif()

if(CONFIG_BT_MESH_FRIEND)
  zephyr_iterable_section(NAME bt_mesh_friend_cb KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
endif()

if(CONFIG_BT_MESH_LOW_POWER)
  zephyr_iterable_section(NAME bt_mesh_lpn_cb KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
endif()

if(CONFIG_BT_MESH_PROXY)
  zephyr_iterable_section(NAME bt_mesh_proxy_cb KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
endif()

if(CONFIG_EC_HOST_CMD)
  zephyr_iterable_section(NAME ec_host_cmd_handler KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
endif()

if(CONFIG_SETTINGS)
  zephyr_iterable_section(NAME settings_handler_static KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
endif()

zephyr_iterable_section(NAME k_p4wq_initparam KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)

if(CONFIG_EMUL)
  zephyr_linker_section(NAME emulators_section GROUP RODATA_REGION)
  zephyr_linker_section_configure(SECTION emulators_section INPUT ".emulators" KEEP SORT NAME ${XIP_ALIGN_WITH_INPUT})
endif()

if(CONFIG_DNS_SD)
  zephyr_iterable_section(NAME dns_sd_rec KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)
endif()

if(CONFIG_PCIE)
  zephyr_linker_section(NAME irq_alloc GROUP RODATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT})
  zephyr_linker_section_configure(SECTION irq_alloc INPUT ".irq_alloc*" KEEP SORT NAME)
endif()

zephyr_linker_section(NAME log_strings KVMA RAM_REGION GROUP RODATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT})
zephyr_linker_section_configure(SECTION log_strings INPUT ".log_strings*" KEEP SORT NAME)

zephyr_linker_section(NAME log_const KVMA RAM_REGION GROUP RODATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT})
zephyr_linker_section_configure(SECTION log_const INPUT ".log_const_*" KEEP SORT NAME)

zephyr_linker_section(NAME log_backends KVMA RAM_REGION GROUP RODATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT})
zephyr_linker_section_configure(SECTION log_backends INPUT ".log_backends.*" KEEP)

zephyr_iterable_section(NAME shell KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)

zephyr_linker_section(NAME shell_root_cmds KVMA RAM_REGION GROUP RODATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT})
zephyr_linker_section_configure(SECTION shell_root_cmds INPUT ".shell_root_cmd_*" KEEP SORT NAME)

zephyr_linker_section(NAME font_entry KVMA RAM_REGION GROUP RODATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT})
zephyr_linker_section_configure(SECTION font_entry INPUT "._cfb_font.*" KEEP SORT NAME)

zephyr_iterable_section(NAME tracing_backend KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)

zephyr_linker_section(NAME zephyr_dbg_info KVMA RAM_REGION GROUP RODATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT})
zephyr_linker_section_configure(SECTION zephyr_dbg_info INPUT ".zephyr_dbg_info" KEEP)

zephyr_linker_section(NAME device_handles KVMA RAM_REGION GROUP RODATA_REGION NOINPUT ${XIP_ALIGN_WITH_INPUT} ENDALIGN 16)
zephyr_linker_section_configure(SECTION device_handles INPUT .__device_handles_pass1* KEEP SORT NAME PASS 1)
zephyr_linker_section_configure(SECTION device_handles INPUT .__device_handles_pass2* KEEP SORT NAME PASS 2)
