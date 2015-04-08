#!/usr/bin/perl

use strict;
no strict "subs";
use Tk;
use Carp;
use IPC::SharedMem;

use Time::HiRes qw(usleep);

#Get access to shared memory
my $shm = IPC::SharedMem->new($ARGV[0], 256, S_IRWXU) || die $!;

#Global Variables
my $age = 10;
my $gender = "Male";

# Main Window
my $mw = new MainWindow;
$mw->geometry('700x500-200+200');

#GUI Building Area

#frames
my $cntrl_frm = $mw -> Frame();
my $reg_frm = $mw->Frame();
my $stack_frm = $mw->Frame();
my $io_frm = $mw->Frame();


#controls
my $step = $cntrl_frm -> Button(-text=>"Step", -command=>\&step);
my $steps_lbl = $cntrl_frm -> Label(-text=>"# of steps:");
my $steps_entry = $cntrl_frm -> Entry(-width=>3);
my $quit = $cntrl_frm -> Button(-text=>"Quit", -command=>\&quit);
my $reset = $cntrl_frm -> Button(-text=>"Reset", -command=>\&reset);
my $continue = $cntrl_frm -> Button(-text=>"Continue", -command=>\&continue);

#register display
my $reg_lab = $reg_frm -> Label(-text=>"Registers:");
my $reg_dat = $reg_frm -> Text(-width=>20, -height=>30, -takefocus=>0);
my $reg_srl_y = $reg_frm -> Scrollbar(-orient=>'v', -command=>[yview => $reg_dat]);
$reg_dat -> configure(-yscrollcommand=>['set', $reg_srl_y]);

$reg_dat->insert('end', "pc := 4\nr1 := 16\nr2 := 40\n");


#stack display
my $stack_lab = $stack_frm -> Label(-text=>"Stack:");
my $stack_dat = $stack_frm -> Text(-width=>30, -height=>30, -takefocus=>0);
$stack_dat->tagConfigure('highlighted', 
			 -foreground=>"white",
			 -background=>"blue");

my $stack_srl_y = $stack_frm -> Scrollbar(-orient=>'v', -command=>[yview => $stack_dat]);
$stack_dat -> configure(-yscrollcommand=>['set', $stack_srl_y]);

$stack_dat->insert('end', "1:b := 16\n2:contents b, 0 := 25\n3:contents b, 1 := 6\n");

#console
my $console_lab = $io_frm -> Label(-text=>"Console:");
my $input_lab = $io_frm -> Label(-text=>"Input:");
my $input_entry = $io_frm -> Entry();
my $io_box = $io_frm -> Text(-height=>28, -width=>20, -takefocus=>1);

$io_box->insert('end', ">6\n6\n->49\n");



my $lab = $cntrl_frm -> Label(-text=>"Name:");
my $ent = $cntrl_frm -> Entry();
#Age
my $scl = $mw -> Scale(-label=>"Age :",
     -orient=>'v',         -digit=>1,
     -from=>10,        -to=>50,
		       -variable=>\$age,    -tickinterval=>10);

#Gender
my $frm_gender = $mw -> Frame();
my $lbl_gender = $frm_gender -> Label(-text=>"Sex ");
my $rdb_m = $frm_gender -> Radiobutton(-text=>"Male",  
				       -value=>"Male",  -variable=>\$gender);
my $rdb_f = $frm_gender -> Radiobutton(-text=>"Female",
				       -value=>"Female",-variable=>\$gender);


my $but = $mw -> Button(-text=>"Push Me", -command =>\&push_button);

#Text Area
my $textarea = $mw -> Frame();
my $txt = $textarea -> Text(-width=>40, -height=>10);
my $srl_y = $textarea -> Scrollbar(-orient=>'v',-command=>[yview => $txt]);
my $srl_x = $textarea -> Scrollbar(-orient=>'h',-command=>[xview => $txt]);
$txt -> configure(-yscrollcommand=>['set', $srl_y],
		  -xscrollcommand=>['set',$srl_x]);

#Geometry Management
$step -> grid(-row=>1,-column=>1);
$steps_lbl -> grid(-row=>2, -column=>1);
$steps_entry -> grid(-row=>2, -column=>2);
$continue -> grid(-row=>1, -column=>2);
$reset -> grid(-row=>1, -column=>3);
$quit -> grid(-row=>1, -column=>4);
$cntrl_frm -> grid(-row=>1,-column=>1,-columnspan=>7);

$reg_lab -> grid(-row=>1, -column=>1);
$reg_dat -> grid(-row=>2, -column=>1);
$reg_srl_y -> grid(-row=>2, -column=>2, -sticky=>"ns");
$reg_frm -> grid(-row=>2, -column=>1,-columnspan=>2);


$stack_lab -> grid(-row=>1, -column=>1);
$stack_dat -> grid(-row=>2, -column=>1);
$stack_srl_y -> grid(-row=>2, -column=>2, -sticky=>"ns");
$stack_frm -> grid(-row=>2, -column=>3,-columnspan=>2);

$console_lab ->grid(-row=>1, -column=>1,-columnspan=>2);
$io_box -> grid(-row=>2, -column=>1,-columnspan=>2);
$input_lab -> grid(-row=>3, -column=>1);
$input_entry -> grid(-row=>3, -column=>2);
$io_frm -> grid(-row=>2, -column=>5, -columnspan=>2);

receive_update("");
MainLoop;

## Functions
sub step{
    my $steps = $steps_entry->get();

    if ($steps eq "") {
	$steps = "1"
    }
    my $message = "step ".$steps."\0";
    $shm->write($message, 0, length $message);
    receive_update("step");
}

sub continue{
    my $message = "continue\0";
    $shm->write($message, 0, length $message);
    receive_update("continue");
}

sub quit{
    my $message = "quit\0";
    $shm->write($message, 0, length $message);
    exit;
}

sub reset{
    my $message = "reset\0";
    $shm->write($message, 0, length $message);
    receive_update("reset");
}

sub receive_update{
    my $prev_command = shift;
    my $message = "";
    my $combined = "";

    #read data 
    #until we receive 'end' flag
    while ("pigs" ne "fly") {
	#wait for 'ready' flag
	my $char = $shm->read(0, 1);
	until ($char eq "r") {
	    usleep(10);
	    $char = $shm->read(0, 1);
	}

	#read characters until null char
	$message = "";
	my $i = 1;
	until ($char eq "\0") {
	    $char = $shm->read($i, 1);
	    $message .= $char;
	    $i++;
	}

	#add to full message
	$combined .= $message;
	$shm->write("\0", 0, 1);
	
	if ($shm->read(1,1) eq "e") {
	    last;
	}
    }

    my @boxes = split "_", $combined;

    $reg_dat->delete('1.0', 'end');
    $reg_dat->Insert($boxes[0]);

    $stack_dat->delete('1.0', 'end');
    my ($pc) = ($boxes[0] =~ /^pc: (\d+)/);
    my $i = 0;

    for (split "\n", $boxes[1]) {
	if ($i == $pc) {
	    $stack_dat->insert('end', $_."\n", 'highlighted');
	} else {
	    $stack_dat->insert('end', $_."\n");
	}
	$i++;
    }

#    $stack_dat->FindAll(-regex, -nocase, "$pc\: [^\n]+\n");

 #   if ($stack_dat->tagRanges('sel')) {
#	print "found ranges\n";
#	my %startfinish  = $stack_dat->tagRanges('sel');
#	foreach(sort keys %startfinish) {
#	    $stack_dat->tagAdd("foundtag", $_, $startfinish{$_});
#	}
 #       $stack_dat->tagRemove('sel', '1.0', 'end');
 #   } else {
#	print "no ranges\n";
 #   }
}
