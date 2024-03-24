#!/bin/sh -e

##########################################################################
#   Description:
#       Fetch Yeast sample data and create symlinks with descriptive names
##########################################################################

usage()
{
    printf "Usage: $0 replicates\n"
    printf "Example: $0 10\n"
    exit 1
}


##########################################################################
#   Main
##########################################################################

if [ $# != 1 ]; then
    usage
fi
replicates=$1

if [ $replicates -lt 3 ]; then
    printf "Sample count must be at least 3.\n"
    exit 1
fi

# Document software versions used for publication
uname -a
fasterq-dump --version || true
pwd

raw=Results/01-fetch/Raw
raw_renamed=Results/01-fetch/Raw-renamed
mkdir -p $raw $raw_renamed

# Link raw files to WT-rep or SNF-rep to indicate the biological condition
# Link raw files to condX-repYY for easy and consistent scripting
# I usually make cond1 the control (e.g. wild-type) or first time point
cond_num=1
for condition in WT SNF2; do
    # Select $replicates replicates
    # Get one technical replicate from each biological replicate
    # Col 2 (Lane) indicates technical rep, use samples where Lane = 1
    # Col 3 is SNF2 mutant or WT
    # Col 4 is biological replicate
    awk -v replicates=$replicates -v condition=$condition \
	'$2 == 1 && $3 == condition && $4 <= replicates' \
	ERP004763_sample_mapping.tsv > $condition.tsv
    printf "$condition:\n"

    for sample in $(awk '{ print $1 }' $condition.tsv); do
	fq="$sample.fastq.gz"
	biorep=$(awk -v sample=$sample '$1 == sample { print $4 }' $condition.tsv)
	# Use 2 digits for all replicates in filenames for easier viewing
	biorep=$(printf "%02d" $biorep)
	if [ ! -e $raw/$fq ]; then
	    # Use rsync if possible on local test platforms.  May not have
	    # sra-tools and pulling from coral saves a lot of bandwidth.
	    printf "Downloading $sample = $condition-$biorep = cond$cond_num-rep$biorep...\n"
	    fasterq-dump --progress --force --outdir $raw $sample
	    printf "Compressing...\n"
	    # Background so next download can start
	    gzip $raw/$sample.fastq &
	fi
	(cd $raw_renamed && ln -fs ../Raw/$fq $condition-$biorep.fastq.gz)
	(cd $raw_renamed && ln -fs ../Raw/$fq cond$cond_num-rep$biorep.fastq.gz)
    done
    rm -f $condition.tsv
    cond_num=$(($cond_num + 1))
done
ls -l $raw
ls -l $raw_renamed
