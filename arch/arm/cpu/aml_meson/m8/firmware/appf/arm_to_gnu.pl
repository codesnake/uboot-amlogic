#!/usr/bin/perl

#
# Copyright (C) 2008-2010 ARM Limited                           
#
# This software is provided 'as-is', without any express or implied
# warranties including the implied warranties of satisfactory quality, 
# fitness for purpose or non infringement.  In no event will  ARM be 
# liable for any damages arising from the use of this software.
#
# Permission is granted to anyone to use, copy and modify this software for 
# any purpose, and to redistribute the software, subject to the following 
# restrictions: 
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.                                       
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
#

# This script is designed to convert ARM UAL code to GNU assembler format.

# What differs between ARM's format and the GNU format?:
# 001) Anything (non comment) that is in column zero is a label and should
#      be ":" terminated, unless it is a register name in an RN directive
#
# 002) Comments are ";" instead of "@" (unless the ; is part of a quoted string)
#
# 003) RN directives are converted to .reg, DN to .dn and QN to .qn
#
# 004) GET/INCLUDE <file> are converted to .include "<file>", although this 
#      currently will erroneously catch GET/INCLUDE in strings.
#
#


use strict;

my ($infile, $outfile, $IN, $OUT, $line, @in, $linenumber, %register_aliases, @argument_names);
my (@overriden_neon_instructions);

$infile  = $ARGV[0];
$outfile = $ARGV[1];

#================================================================================
# Whatever you do, don't overwrite the armCOMM_gnu_s.h file
#================================================================================

exit if ($infile=~m/armCOMM_arm_s.h/);
exit if ($outfile=~m/armCOMM_gnu_s.h/);

  

#================================================================================
# We have to do two passes to see if we can work around a bug in the assembler
# to do with overriding Neon registers. We want to get a list of all Neon (V) 
# instructions which have a type overrider

open($IN, '<'.$infile) or die ("Can't open input $infile");
@in = <$IN>;
close($IN);

foreach $line (@in)
{
    if ($line=~m/\s*V[A-Za-z0-9]*\.[USFusf][0-9]*\s/)
    {
        push(@overriden_neon_instructions, $line);
    }
}


#================================================================================


open($IN, '<'.$infile) or die ("Can't open input $infile");
@in = <$IN>;
close($IN);

open($OUT, '>'.$outfile) or die("Can't open output $outfile");
binmode($OUT);

print $OUT  "        @ Created by arm_to_gnu.pl from $infile\n";
print $OUT  "        .syntax unified\n\n";

#=============================================================
# Aligns comments to a sensible 4 space boundary
#=============================================================
sub print_with_spacing
{
    my ($file, $label, $instruction, $comment)=@_;
    my ($length, $num_spaces, $spaces);

 #   print "*".$label."*".$instruction."*".$comment."*\n";
    
    if (($label ne "") && ($label ne ":"))
    {
        # First ensure that there is only one space (and a colon) 
        # at the end of the label - there can't be any at the start).
        $label=~m/(.*)\s*/;
        $label=$1;

        $length = length($label);
        $num_spaces=4-($length % 4);
        $num_spaces+=4 if ($length<8); 
        $num_spaces+=4 if ($length<4); 
        $spaces = " " x $num_spaces;
        $instruction=$label.$spaces.$instruction;
    }
    else
    {
        # Stick 8 spaces on the start.
        $instruction=~s/^\s*//g;
        $instruction="        ".$instruction;
    }

    # First ensure that there is only one space at the end of 
    # the instruction.
    $spaces="";
    if ($instruction=~m/\S/)
    {
        $instruction=~m/(.*)\s*/;
        $instruction=$1." ";
        
        $length = length($instruction);
        if ($length < 60)
        {
            $num_spaces=4-($length % 4);
            $num_spaces+=4 if ($length<8); 
            $num_spaces+=4 if ($length<4); 
            $spaces = " " x $num_spaces;
        }
    }
#    print $file "*".$instruction."*".$spaces."*".$comment."\n";
    print $file $instruction.$spaces.$comment."\n";
}


my ($label, $instruction, $comment, $character, @characters);
my ($inquotes, $inlabel, $inbackslash, $incomment, $inmacro,$inmacro1);
my ($structure_name, $structure_offset);


#===============================================================
# This processes a line at a time, anything over multiple lines 
# will be problematic.
#===============================================================
$linenumber=0;
$inmacro=$inmacro1=0;
$structure_name="";
$structure_offset=0;

foreach $line (@in)
{
    $linenumber++;
    
    # Remove any line endings
    $line=~s/[\n\f\r]//g;
    
    $incomment=0;
    $inquotes=0;
    $inbackslash=0;
    $inlabel=1;
    $label=$instruction=$comment="";
      

    #===============================================================
    # Now run a state machine to check for comments etc, by the
    # end of this section we should have a label (if any), the bulk of any
    # instruction (if any), and a trailing comment (if any).
    # These machines just run over a single line and reset on the 
    # next, the later macro one runs over multiple lines
    #===============================================================

    @characters=split("", $line);
    foreach $character (@characters)
    {
        if ($incomment)
        {
            # Once we're in a comment we can't come out.
            $comment.=$character;
        }
        elsif ($inquotes)
        {
            $instruction.=$character;
            if ($character eq "\"")
            {
                $inquotes=0;
            }
        }
        elsif ($inlabel)
        {
            # We can only be in a label up to the first white space character, or
            # until there is a comment starting - we assume that there can't be
            # backslashed characters, or quotes in a label...
            if ($character=~m/\s/)
            {
                $inlabel=0;
            }
            elsif($character eq ";")
            {
                $incomment=1;
                $inlabel=0;
            }
            else
            {
                $label.=$character;
            }
        }
        elsif ($inbackslash)
        {
            $inbackslash=0;
            $instruction.=$character;
        }
        else
        {
            # We're not in a comment, label, or quote.
            if ($character eq ";")
            {
                $incomment=1;
            }
            elsif($character eq "\\")
            {
                $inbackslash=1;
                $instruction.=$character;
            }
            elsif ($character eq "\"")
            {
                $inquotes=1;
                $instruction.=$character;
            }
            else
            {
                $instruction.=$character;
            }
        }
        
    }
    if ($comment ne "")
    {
        $comment="@".$comment;
    }
        
    #	print $OUT " LABEL $label LABEL    INST $instruction INST    COMM $comment COMM\n";
    
    #===============================================================
    # We now have a label (if any), the bulk of any instruction 
    # (if any), and a trailing comment (if any).
    # We'll now go through any special substitutions we need.
    #===============================================================


    #===============================================================
    #
    # ************ GNU BUG WORK AROUND #1 ********************
    #
    # The current (2008Q1) GNU assembler cannot handle 
    # upper case characters in NEON register aliases...
    # So we'll have to lower case them all, and cross our fingers!
    #===============================================================
    my ($reg,$loreg);
    foreach $reg (keys(%register_aliases))
    {
        $loreg=lc($reg);
        $instruction=~s/(\W)$reg(\W|$)/\1$loreg\2/g;
    }
    
    # ************ GNU BUG WORK AROUND #2 ********************
    #
    # The current (2008Q1) GNU assembler cannot handle 
    # overriding the register alias types in the instruction
    #===============================================================

    if ($line=~m/\s*V[A-Za-z0-9]*\.[USFusf][0-9]*\s/)
    {
        foreach $reg (keys(%register_aliases))
        {
            $reg=lc($reg);
            $instruction=~s/(\W)$reg(\W|$)/\1${reg}_notype\2/g;
        }
    }
    
    # ************ GNU BUG WORK AROUND #3 ********************
    #
    # The current (2010Q1) GNU assembler cannot handle VMRS/VMSR
    # So we switch them to MRS/MSR... seems to work.
    #===============================================================

    $instruction=~s/^\s*VMRS(.*)/        MRS$1/ig;
    $instruction=~s/^\s*VMSR(.*)/        MSR$1/ig;
    

    #--------------------------------------------------------------------------
    # The only time we are going to modify the comment is if it contains 
    # an EQU declaration (as these are likely to be commented out
    #--------------------------------------------------------------------------
    $comment=~s/(\S+)\s+EQU\s+(\S+)/.equ $1, $2/g;

    #--------------------------------------------------------------------------
    # First go any general substitutions before spotting more major constructs
    
    #--------------------------------------------------------------------------
    
    # Convert hard tabs at start of line to spaces
    #  - for some reason perl's \s doesn't like them
    $instruction=~s/^(\t*)/        /g;
    
    # Convert {TRUE} and {FALSE} to 1 and 0
    $instruction=~s/{TRUE}/ 1 /ig;
    $instruction=~s/{FALSE}/ 0 /ig;
    
    # Hexadecimal constants prefaced by 0x
    $instruction=~s/#&/#0x/g;
    
    # Convert :OR: to |
    $instruction=~s/:OR:/ | /g;
    
    # Convert :AND: to &
    $instruction=~s/:AND:/ & /g;
    
    # Convert :NOT: to ~
    $instruction=~s/:NOT:/ ~ /g;
    
    # Convert :SHL: to <<
    $instruction=~s/:SHL:/ << /g;
    
    # Convert :SHR: to >>
    $instruction=~s/:SHR:/ >> /g;
    
    #Convert n-base to binary
    $instruction=~s/2_/0b/g;

    #Convert local label names e.g. %f0 to 0f
    $instruction=~s/%([fFbB])(\d+)/$2$1/g;
    

    #Convert ENTRY to _start
    $instruction=~s/^\s*ENTRY//g;#_start:/g;
    
    $instruction=~s/entrypoint/entrypoint_dummy/g;
    
    # Handle IF :LNOT::DEF:
    $instruction=~s/^\s*IF\s*\:LNOT\:\s*\:DEF\:\s*(.*)/.ifndef $1/g;
    
    # Handle IF :DEF:
    $instruction=~s/^\s*IF\s*\:DEF\:\s*(.*)/.ifdef $1/g;
    
    # Convert IF and [ to .if
    $instruction=~s/^\s*(IF|\[)(\s)(.*)/.if$2$3 /g;

    # Convert ENDIF and ] to .endif
    $instruction=~s/^\s*(ENDIF|\])($|[^A-Za-z_])/.endif$2/g;
    
    # Convert ELSEIF to .elseif
    $instruction=~s/^\s*ELSEIF($|[^A-Za-z_])/.elseif$1/g;
    
    # Convert ELSE and | to .else
    $instruction=~s/^\s*(ELSE|\|)($|[^A-Za-z_])/.else$2/g;
    
    #Import is not used
    $instruction=~s/^\s*IMPORT\s+(\w*)/ /g;
    
    #Convert FUNCTION and ENDFUNC to .func and .endfunc
    $instruction=~s/FUNCTION($|[^A-Za-z_])/.func$1/g;
    $instruction=~s/ENDFUNC($|[^A-Za-z_])/.endfunc$1/g;
    
    # Convert INCLUDE to .INCLUDE "file"
    if ($instruction=~m/INCLUDE(\s*)(.*)/)
    {
        my ($space, $filename);
        $space=$1;
        $filename=$2;
        if ($filename=~s/_arm_s\.h/_gnu_s\.h/)
        {
        }
        else
        {
            $filename=~s/_s\.h/_gnu_s\.h/;
        }
        $instruction=~s/INCLUDE(\s*)(.*)/.include $space\"$filename\"/;
    }
    
    # Code directive (ARM vs Thumb)
    $instruction=~s/CODE([0-9][0-9])/.code $1/;   
    
    #Convert ALIGN to .align
    $instruction=~s/^\s*ALIGN\s+(\w*)/.align $1/g;

    # Make function visible to linker, and make additional symbol with
    # prepended underscore
    $instruction=~s/^\s*EXPORT\s+\|(\w*)\|/.global _$1\n.global $1/;
    $instruction=~s/^\s*EXPORT\s+(\w*)/.global $1/;
    
    # No vertical bars required; make additional symbol with prepended
    # underscore 
    $instruction=~s/^\|(\w+)\|/_$1\n$1:/g;
    
    
    # Convert INFO to .print
    $instruction=~s/\sINFO([^,]*),(.*) /.print $2/g;    
    
    # MAIN directive
    $instruction=~s/__main/main/g;
    
    # Convert PRESERVE8 etc. to Tag_ABI_align_preserved attributes
    $instruction=~s/PRESERVE8\s*$/.eabi_attribute Tag_ABI_align_preserved, 1/g;
    $instruction=~s/REQUIRE8\s*$/.eabi_attribute Tag_ABI_align_needed, 1 $1/g;
    $instruction=~s/PRESERVE8\s+(\w+)/.eabi_attribute Tag_ABI_align_preserved, $1/g;
    $instruction=~s/REQUIRE8\s+(\w+)/.eabi_attribute Tag_ABI_align_needed, $1/g;

    #Space Directive
    $instruction=~s/\sSPACE\s+(.*)/\n.space $1,0/;
    
    #Convert DCD to .word
    $instruction=~s/\sDCD\s+(.*)/\n.word $1/;
   
    #Convert DCB to .string
    $instruction=~s/\sDCB\s+\"(.*)\"(.*)/\n.string "$1"/;

    # Convert Macro exits
    $instruction=~s/\sMEXIT($|[^A-Za-z_])/.exitm$1/;

    # Convert Macro ends, and come out of our macro state machine
    if ($instruction=~s/\sMEND($|[^A-Za-z_])/.endm$1/)
    {
        $inmacro=$inmacro1=0;
    }
 
    # Handle parameter names inside macros (but not the first line)
    if ($inmacro && (!$inmacro1))
    {
        $instruction=~s/\$/\\/g;
    }

    # Convert END to .end - must occur after swap of ENDIF and of MEND!!
    # But only if it is not in an included header file - as in GNU this
    # stops all compilation, not just of that file. We'll assume that 
    # anything with .h is included, and .s is not included...
    if ($infile=~m/\.h/)
    {
        $instruction=~s/^\s*END($|\s)/$1/g;
    }
    else
    {
        $instruction=~s/^\s*END($|\s)/.end$1/g;
    }

    #--------------------------------------------------------------------------
    # Handle the Macros
    #--------------------------------------------------------------------------
    # Convert Macro and enter our macro state machine, the next
    # valid line should be a macro label name
    if ($instruction=~m/MACRO/)
    {
        # The next line should be the macro label name,
        # so we'll set the flag for being in a macro, and
        # the flag for looking for the first line in a macro
        $inmacro=$inmacro1=1;
    }
    elsif($inmacro1)
    {
        if ($instruction ne "")
        {
            $instruction=~m/\s*(\S*)\s*(.*)/;
            my ($macroname, $macroparams);
            $macroname=$1;
            $macroparams=$2;
            
            if ($label ne "")
            {
                $macroparams="$label, $macroparams";
            }
            $macroparams=~s/\$//g;
            print_with_spacing($OUT, "", ".macro $macroname $macroparams", $comment);
           $inmacro1=0;
        }
    }
    #--------------------------------------------------------------------------
    # Have we got an symbol declaration (GBLA, GBLL, GBLS, LCLA, LCLL, LCLS)
    #--------------------------------------------------------------------------
    elsif ($instruction=~m/(^|\s)(LCL|GBL)(A|L|S)\s*(\S*)/)
    {
        my ($local, $type, $default, $name);
        $local=$2;
        $type=$3;
        $name=$4;
        
        # Work out the default value to set (ARM asm does it implicitly)
        $default="0" if ($type eq "A");
        $default="FALSE" if ($type eq "L");
        $default="\" \"" if ($type eq "S");

        if ($type ne "S")
        {
            if ($local eq "LCL")
            {
                print_with_spacing($OUT, "", "    .set $name, $default", $comment);
            }
            else
            {
                print_with_spacing($OUT, "", "    .global $name", $comment);
                print_with_spacing($OUT, "", "    .set $name, $default", "");
            }
        }
    }
    #--------------------------------------------------------------------------
    # Have we got an symbol setting (SETA, SETL, SETS)
    #--------------------------------------------------------------------------
    elsif ($instruction=~m/(^|\s)SET(A|L|S)\s*(\S*)/)
    {
        my ($type, $expression);
        $type=$2;
        $expression=$3;
        
        # Remove any squiggly brackets from ARM predefined expressions
        $expression=~s/[\{\}]//g;
        
        if ($type ne "S")
        {
            print_with_spacing($OUT, "", "    .set $label, $expression", $comment);
        }
    }    


    #--------------------------------------------------------------------------
    # Have we got an RN (or DN or QN) register naming directive? That's a special case 
    # as the register name looks like a label, but isn't.
    #
    # *** Currently there is a bug in the GNU assembler which means that 
    # *** Neon register alias names must be converted to lower case...
    #
    #--------------------------------------------------------------------------
    elsif ($instruction=~m/(^|\s)(RN|DN|QN)\s*(\S*)/)
    {
        my ($register, $armtype, $gnutype);
        $armtype=$2;
        $register=$3;
        if ($label eq "")
        {
            print "RN/DN/QN directive without register name label at line: $linenumber\n";
        }
        if ($register=~m/^\d+/)
        {
           $register="r".$register;
        }
        $gnutype = ".req" if ($armtype eq "RN");
        $gnutype = ".dn" if ($armtype eq "DN");
        $gnutype = ".qn" if ($armtype eq "QN");
        
        #========================================================
        # This catches a GNU bug, but will be very hacky
        # We have to convert Neon register aliases to lower case.
        #========================================================
        if (($gnutype eq ".dn") || ($gnutype eq ".qn"))
        {
            $register_aliases{$label} = $register;
            print "Neon Register Alias: $label\n";
            $label=lc($label);
        }
        

        print_with_spacing($OUT, $label, $gnutype." ".$register, $comment);
        
        # This catches a second GNU AS bug - we need to output duplicate 
        # register aliases without the types for cases where the 
        # register goes on to be used in an instruction which overrides the types.
        my ($found_problem, $prob_instruction);
        $found_problem=0;
        foreach $prob_instruction (@overriden_neon_instructions)
        {
            $prob_instruction=lc($prob_instruction);
            if ($prob_instruction=~m/$label/)
            {
                $found_problem=1;
            }
        }
        
        if ($found_problem)
        {
            $register=~s/\.[USF][0-9]*//g;
            print_with_spacing($OUT, $label."_notype", $gnutype." ".$register, $comment);
        }

    }
    #--------------------------------------------------------------------------
    # Handle EQU
    #--------------------------------------------------------------------------
    elsif($instruction=~m/\s*EQU\s+(\S*)/)
    {
        my ($expression);
        $expression=$1;
        
        if (!defined($expression))
        {
            print "Conversion of EQU can't find an expression at line: $linenumber\n";
        }
        elsif ($expression=~/,/)
        {
            print "Conversion of EQU expression can't handle commas yet at line: $linenumber\n";
        }
        print $OUT "    .equ $label, $expression";
        
        # Finish off with any comments, and a newline.
        if ($comment ne "")
        {
            print $OUT " \@$comment";
        }
        print $OUT "\n";
    }
        
    #--------------------------------------------------------------------------
    # OpenMAX Specific - Handle Structure macros
    #--------------------------------------------------------------------------
    elsif($instruction=~m/M_STRUCT\s*(\S*)/)
    {
        $structure_name=$1;
        $structure_offset=0;
        if ($comment ne "")
        {
            print $OUT " \@$comment\n";
        }
    }
    elsif($instruction=~m/M_ENDSTRUCT/)
    {
        print $OUT "    .equ sizeof_".$structure_name.",".$structure_offset." ";

        if ($comment ne "")
        {
            print $OUT " \@$comment";
        }
        print $OUT "\n";
        
        $structure_name="";
        $structure_offset=0;
    }
    elsif($instruction=~m/M_FIELD\s*/)
    {
        my ($field, $size, $number);
        $instruction=~m/M_FIELD\s*(\S*)\s*,\s*(\S*)/;
        $field=$1;
        $size=$2;
        $number=0;
        if ($instruction=~m/M_FIELD\s*(\S*)\s*,\s*(\S*)\s*,\s*(\S*)/)
        {
            $number=$3;
        }
        
        
        print "STRUCT=$structure_name FIELD=$field  SIZE=$size, NUMBER=$number\n";
        
        # Declare a structure field
        # The field is called $sname_$fname
        # $size   = the size of each entry, must be power of 2 
        # $number = (if provided) the number of entries for an array
        if (($structure_offset & ($size-1)) != 0)
        {
           $structure_offset += ($size - ($structure_offset & ($size-1)));
        }

        print $OUT "    .equ ".$structure_name."_".$field.", ".$structure_offset;

        if ($number == 0)
        {
            $structure_offset+=$size;
        }
        else
        {
            $structure_offset+=$size*$number;
        }

        if ($comment ne "")
        {
            print $OUT " \@$comment";
        }
        print $OUT "\n";
        
    }        

    #--------------------------------------------------------------------------
    # OpenMAX Specific - Remember ARGument names, will come in useful in 
    # load instructions.
    #--------------------------------------------------------------------------
    elsif($instruction=~m/(\s*)M_ARG(\s*)(.*)/)
    {
        my ($argname);
        print_with_spacing($OUT, $label.":", "$1M_ARG$2$3", $comment);
        $argname=$3;
        $argname=~s/\s*,.*//;
        print "ARGNAME=$argname\n";
        push (@argument_names, $argname);
    }
    
    #--------------------------------------------------------------------------
    # OpenMAX Specific - Expand M_LDR* and M_STR* macros 
    #--------------------------------------------------------------------------
    # Macro to perform a data access operation
    # Such as LDR or STR
    # The addressing mode is modified such that
    # 1. If no address is given then the name is taken
    #    as a stack offset
    # 2. If the addressing mode is not available for the
    #    state being assembled for (eg Thumb) then a suitable
    #    addressing mode is substituted.
    #
    # On Entry:
    # $i = Instruction to perform (eg "LDRB")
    # $a = Required byte alignment
    # $r = Register(s) to transfer (eg "r1")
    #    ;// $a0,$a1,$a2. Addressing mode and condition. One of:
    #    ;//     label {,cc}
    #    ;//     [base]                    {,,,cc}
    #    ;//     [base, offset]{!}         {,,cc}
    #    ;//     [base, offset, shift]{!}  {,cc}
    #    ;//     [base], offset            {,,cc}
    #    ;//     [base], offset, shift     {,cc}
    
    elsif($instruction=~m/M_(LDR|STR)(|B|SB|H|D)\s*(.*)/)
    {
        my ($instruction, $rest, $r, $a0, $a1, $a2, $a3);
       
        $instruction=$1.$2;
        $rest=$3;
        ($r, $a0, $a1, $a2, $a3) = split (/\s*,\s*/, $rest);
        

        
        print "INSTRUCTION=$instruction,   REST=:$r:$a0:$a1:$a2:$a3:\n";
        
        if ($a0=~m/^\[/)
        {
            if ($a1 eq "")
            {
                print_with_spacing($OUT, $label.":", "$instruction$a3    $r, $a0", $comment);
            }
            else
            {
               # We've got square brackets!
                print $OUT "M_LDR I do hope the assembler doesn't try to assemble this - fix M_LDR in arm_to_gnu.pl\n";
#                IF "$a0":RIGHT:1="]"
#                    IF "$a2"=""
#                        _M_POSTIND $i$a3, "$r", $a0, $a1
#                    ELSE
#                        _M_POSTIND $i$a3, "$r", $a0, "$a1,$a2"
#                    ENDIF
#                ELSE
#                    IF "$a2"=""
#                        _M_PREIND  $i$a3, "$r", $a0, $a1
#                    ELSE
#                        _M_PREIND  $i$a3, "$r", $a0, "$a1,$a2"
#                    ENDIF
#                ENDIF
            }
        }
        else
        {
            print_with_spacing($OUT, $label.":", "M_DATA  $instruction$a1, $r, $a0  ", $comment);
        }

#        MACRO
#        _M_DATA $i,$a,$r,$a0,$a1,$a2,$a3
#        IF "$a0":LEFT:1="["
#            IF "$a1"=""
#                $i$a3   $r, $a0
#            ELSE
#                IF "$a0":RIGHT:1="]"
#                    IF "$a2"=""
#                        _M_POSTIND $i$a3, "$r", $a0, $a1
#                    ELSE
#                        _M_POSTIND $i$a3, "$r", $a0, "$a1,$a2"
#                    ENDIF
#                ELSE
#                    IF "$a2"=""
#                        _M_PREIND  $i$a3, "$r", $a0, $a1
#                    ELSE
#                        _M_PREIND  $i$a3, "$r", $a0, "$a1,$a2"
#                    ENDIF
#                ENDIF
#            ENDIF
#        ELSE
#            LCLA    _Offset
#_Offset     SETA    _Workspace + $a0$_F
#            ASSERT  (_Offset:AND:($a-1))=0
#            $i$a1   $r, [sp, #_Offset]
#        ENDIF


    }

        
#        ;// Load unsigned half word from stack
#        MACRO
#        M_LDRH  $r,$a0,$a1,$a2,$a3
#        _M_DATA "LDRH",2,$r,$a0,$a1,$a2,$a3
#        MEND
        
        
#        MACRO
#        _M_DATA $i,$a,$r,$a0,$a1,$a2,$a3
#        IF "$a0":LEFT:1="["
#            IF "$a1"=""
#                $i$a3   $r, $a0
#            ELSE
#                IF "$a0":RIGHT:1="]"
#                    IF "$a2"=""
#                        _M_POSTIND $i$a3, "$r", $a0, $a1
#                    ELSE
#                        _M_POSTIND $i$a3, "$r", $a0, "$a1,$a2"
#                    ENDIF
#                ELSE
#                    IF "$a2"=""
#                        _M_PREIND  $i$a3, "$r", $a0, $a1
#                    ELSE
#                        _M_PREIND  $i$a3, "$r", $a0, "$a1,$a2"
#                    ENDIF
#                ENDIF
#            ENDIF
#        ELSE
#            LCLA    _Offset
#_Offset     SETA    _Workspace + $a0$_F
#            ASSERT  (_Offset:AND:($a-1))=0
#            $i$a1   $r, [sp, #_Offset]
#        ENDIF
#        MEND



    #--------------------------------------------------------------------------
    # Handle AREA directives
    #--------------------------------------------------------------------------

    elsif ($instruction=~m/\s*AREA\s+/)
    {
        my ($align);
	$align="";
	
        # Try to create a separate .align directive from the AREA alignment

        # This doesn't seem to work - the GNU assembler  (2.19.51.20090709) 
	# seems to react to a .align 10 directive at the start of a section
	# by adding 1024 bytes of padding, which it isn't supposed to.
	# For now just use ALIGN= instructions in the linker script.
	
        if ($instruction=~m/\,\s*ALIGN\s*=\s*(\d+)/)
        {
            # $align = sprintf(".align $1");
            $instruction=~s/\,\s*ALIGN\s*=\s*(\d+)//;
        }	

        # Convert AREA to respective sections 
        $instruction=~s/^\s*AREA\s+(\S+)\s*,\s*CODE/.section $1,\"ax\"/g;
        $instruction=~s/^\s*AREA\s+(\S+)\s*,\s*DATA(.*)NOINIT/.section $1,\"aw\"/g;
        $instruction=~s/^\s*AREA\s+(\S*)\s*,\s*DATA/.section $1,\"aw\"/g;
        print_with_spacing($OUT, $label.":", $instruction, $comment);
        print_with_spacing($OUT, "", $align, "");
    }	
    #--------------------------------------------------------------------------
    # Handle MOV32 pseudo-instruction (can only handle a number, not a symbol!)
    #--------------------------------------------------------------------------

    elsif ($instruction=~m/^\s*MOV32\s*(\w+)\s*,\s*(0x\d+)/i)
    {
        my ($number, $register, $expression1, $expression2);
        $register=$1;
        $number=hex($2);
	if ($number == 0)  { $number = int($2); }
	$expression1 = sprintf("#0x%x", $number & 0xffff);
	$expression2 = sprintf("#0x%x", $number >> 16);
	
	#print("Found a MOV32: $register, $number -> $expression1, $expression2\n");
        print_with_spacing($OUT, $label.":", "    MOVW\t$register, $expression1", "");
        print_with_spacing($OUT, "", "    MOVT\t$register, $expression2", $comment);
    }    


    #--------------------------------------------------------------------------
    # Default is to just output the line
    #--------------------------------------------------------------------------
    
    else
    {
        print_with_spacing($OUT, $label.":", $instruction, $comment);
        
        if ($instruction=~m/M_START/)
        {
            @argument_names=();
        }
    }
    


}

close($OUT);
