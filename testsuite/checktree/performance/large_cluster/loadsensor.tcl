#!/vol2/tools/SW/solaris64/bin/tclsh8.3

# get environment
set SGE_ROOT $env(SGE_ROOT)
set ARCH     $env(ARCH)
set HOST     $env(HOST)

# retrieve hostnames
set f [open "|$SGE_ROOT/bin/$ARCH/qhost -l simhost=$HOST" r]

set hosts ""
while {[gets $f line] > 0} {
   set ahost [lindex $line 0]
   if {[string compare [string range $ahost 0 5] lchost] == 0} {
      lappend hosts $ahost
   }
}

close $f

set init_value 0.5
foreach i $hosts {
   set x($i) $init_value
   set init_value [expr $init_value + 0.5]
}

set random_seed 0
while { [gets stdin line] >= 0 } {
   if {[string compare $line "quit"] == 0} {
      exit
   }

   puts "begin"
  
   foreach i $hosts {
      puts "$i:arch:$ARCH"
      puts "$i:num_proc:1"
      set load [expr sin($x($i)) + 1.0]
      set loadstr [format "%0.2f" $load]
      puts "$i:load_avg:$loadstr"
      puts "$i:np_load_avg:$loadstr"
      set random [expr (5 - ($random_seed % 11)) / 29.0]
#      puts "random = $random"
      set x($i) [expr $x($i) + 0.05 + $random]
#      puts "x($i) = $x($i)"
      incr random_seed [expr 1 + $random_seed % 7]
#      puts "random_seed = $random_seed" 
   }
   
   #set x [expr sin($x)]
   
   puts "end"
}
