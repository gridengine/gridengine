#!/usr/bin/perl

#$ -S /usr/bin/perl
#$ -cwd

print "Word Report for $ARGV[0]\n\n";
print "Occurences\t\tWord\n";
print "===================================================\n";

while (<>) {
	@words = split(/\W+/);
		foreach $word (@words) {
			$word =~ tr/A-Z/a-z/;
			$count{$word}++;
		}
	}

foreach $word (sort by_count keys %count) {
	print "$count{$word}\t\t\t$word\n";
	}

sub by_count {
	$count{$b} <=> $count{$a};
}
