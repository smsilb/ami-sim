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
my $wait_input = 0;#flag to force the user to input something in the console

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

#stack display
my $stack_lab = $stack_frm -> Label(-text=>"Stack:");
my $stack_dat = $stack_frm -> Text(-width=>30, -height=>30, -takefocus=>0);
$stack_dat->tagConfigure('highlighted', 
			 -foreground=>"white",
			 -background=>"blue");

my $stack_srl_y = $stack_frm -> Scrollbar(-orient=>'v', -command=>[yview => $stack_dat]);
$stack_dat -> configure(-yscrollcommand=>['set', $stack_srl_y]);

#console
my $console_lab = $io_frm -> Label(-text=>"Console:");
my $input_lab = $io_frm -> Label(-text=>"Input:");
my $input_entry = $io_frm -> Entry();
my $io_box = $io_frm -> Text(-height=>28, -width=>20, -takefocus=>1);

$input_entry->bind('<Return>', \&input);

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

receive_update("initial");
MainLoop;

## Functions

#Most of the following functions depend on the state of $wait_input
#in that many features are disabled if the program is waiting for the
#user to enter a value


sub step{
#sends a 'step' command to the simulator with the number of instructions
#specified in the $steps_entry widget (default 1)
    if ($wait_input != 1) {
	my $steps = $steps_entry->get();

	if ($steps eq "") {
	    $steps = "1"
	}
	my $message = "step ".$steps."\0";
	$shm->write($message, 0, length $message);
	receive_update("step");
    }
}

sub continue{
#sends a 'continue' command to the simulator
    if ($wait_input != 1) {
	my $message = "continue\0";
	$shm->write($message, 0, length $message);
	receive_update("continue");
    }
}

sub quit{
#sends a 'quit' command to the simulator and immediately exits
    if ($wait_input != 1) {
	my $message = "quit\0";
	$shm->write($message, 0, length $message);
	exit;
    }
}

sub reset{
#sends a 'reset' command to the simulator
    my $message;

    #if we're waiting for input, send garbage to the simulator
    #to first unfreeze it, then reset.
    if ($wait_input == 1) {
	$message = "i0\0";
	$shm->write($message, 0, length $message);
	$wait_input = 0;
	receive_update("fake input");
    }

    $message = "reset\0";
    $shm->write($message, 0, length $message);
    receive_update("reset");
}

sub input{
#sends the input from the $input_entry widget to the simulator
#and inserts it into the console output box to simulate the console
#resets the $wait_input flag since we have now sent input
    if ($wait_input == 1) {
	my $message = $input_entry->get();
	if (length $message > 0 && $message =~ /^\d+$/) {
	    $io_box->insert('end', "$message\n");
	    $message = "i$message\0";
	    $shm->write($message, 0, length $message);
	    $wait_input = 0;
	    receive_update("input");
	} else {
	    $io_box->insert('end', " Enter a valid number\n>");
	}
    }
}

sub receive_update{
#waits for, and then processes, data from the simulator

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
	
	#an 'e' indicates that this is the end of the data
	if ($shm->read(1,1) eq "e") {
	    last;
	}
    }

    #data is separated into 3 groupd (1 for each data box)
    #by '~' characters
    my @boxes = split "~", $combined;

    #store reg box scroll position, delete data
    #reinsert new data, and rescroll box
    my ($first, $last) = $reg_srl_y->get();
    $reg_dat->delete('1.0', 'end');
    $reg_dat->Insert($boxes[0]);
    $reg_dat->yviewMoveto($first);

    #store stack box scroll position, delete data
    ($first, $last) = $stack_srl_y->get();
    $stack_dat->delete('1.0', 'end');

    #get the program counter from the register box for use
    #in highlighting the next instruction
    my ($pc) = ($boxes[0] =~ /^pc: (\d+)/);

    #loop through all instruction lines, strip out unsupported characters
    #and insert the line into the stack data box. if it is the
    #next instruction, highlight it
    my $i = 0;

    for (split "\n", $boxes[1]) {
	my ($line) = ($_ =~ /(\d+: [a-zA-Z0-9:=\-\*\+\/\,_ ]+)/);
	if ($i == $pc) {
	    $stack_dat->insert('end', "$line\n", 'highlighted');
	} else {
	    $stack_dat->insert('end', "$line\n");
	}
	$i++;
    }

    #rescroll the stack data box
    $stack_dat->yviewMoveto($first);

    #if we've received an input indicator, we must wait
    #until a value has been inputed before doing other actions
    if ($boxes[2] =~ /^\>/) {
	$wait_input = 1;
    }
    
    #insert either a '>' input indicator or an
    #'outputed' number in the for '-> NUM'
    $io_box->insert('end', $boxes[2]);
}

