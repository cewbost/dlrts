#makefile for building binaries

-include make_config

$(binary): $(obj_files)
	$(CC) -L$(bin_dir) -L$(lib_dir) -Wl,-rpath=\$$ORIGIN/ $(l_options) -o $(binary) $(obj_files) $(libraries)

$(obj_dir)%.o: $(src_dir)%.cpp
	$(CC) -c $(c_options) -o $@ $<

-include ($(dep_files))
