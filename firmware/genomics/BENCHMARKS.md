# Benchmarks

Commands used for the genomics benchmarks of the paper *"Energy-efficient genomic analysis pipelines using open computational storage devices"*.

The same commands were run on the host (compute node) and on the CSDs. Only the number of threads differs: 24 on the host (AMD Ryzen 9 3900X) and 8 on a CSD (RK3588). Each benchmark was run four times. Run time is the wall-clock time reported by `time`. Energy was measured at the wall socket with an EMU Check USB power meter (see the paper, Section 4.7).

## Software

| Application | Software | Version | Source |
|---|---|---|---|
| Quality control | FastQC | 0.12.1 | https://www.bioinformatics.babraham.ac.uk/projects/fastqc/ |
| Alignment | Minimap2 | 2.28 | https://github.com/lh3/minimap2 |
| Variant calling | GraphTyper | 2.7.7 | https://github.com/DecodeGenetics/graphtyper |
| Phasing & imputation | GLIMPSE2 | 2.0.0 | https://github.com/odelaneau/GLIMPSE |
| Checksums | GNU coreutils (`md5sum`, `sha256sum`) | 8.32 | https://www.gnu.org/software/coreutils/ |

Minimap2 and GraphTyper were compiled from source on both architectures. FastQC runs on the Java VM.

GLIMPSE2 on ARM: x86 SIMD intrinsics were replaced with portable equivalents using SIMDe (https://github.com/simd-everywhere/simde). Branch `simde_integration`, fork: https://github.com/rwk-unil/GLIMPSE/tree/simde_integration commit ea1ab10. Upstream GLIMPSE2 has since added aarch64 support (https://github.com/odelaneau/GLIMPSE/pull/286).

## Data

All datasets are public; see the GIAB data indexes (https://github.com/genome-in-a-bottle/giab_data_indexes) and the 1000 Genomes data portal (https://www.internationalgenome.org/data-portal/data-collection).

| Benchmark | Data | Files |
|---|---|---|
| QC, checksums | HG002, Illumina 2x250 bp WGS, one flow cell, 68 gzip-compressed FASTQ files (151 GB) | `D1_S1_L00{1,2}_R{1,2}_0XX.fastq.gz` https://ftp-trace.ncbi.nlm.nih.gov/ReferenceSamples/giab/data_indexes/AshkenazimTrio/sequence.index.AJtrio_Illumina_2x250bps_06012016_updated.HG002 |
| Alignment | One lane of the flow cell above, 67,832,182 read pairs (11.6 GB) | `D1_S1_L001_R1_001.fastq.gz`, `D1_S1_L001_R2_001.fastq.gz` |
| Alignment reference | GRCh38 | https://ftp.ncbi.nlm.nih.gov/genomes/all/GCF/000/001/405/GCF_000001405.26_GRCh38/GCF_000001405.26_GRCh38_genomic.fna.gz |
| Variant calling | HG001, Illumina 2x150 bp WGS, 30x (148 GB) | `RMNISTHS_30xdownsample.bam` (+ `.bai`) https://github.com/genome-in-a-bottle/giab_data_indexes/blob/master/NA12878/alignment.index.NA12878_HiSeq_downsampled30X_GRCh37_10262015 |
| Variant calling reference | GRCh37 (1000 Genomes) | http://ftp.1000genomes.ebi.ac.uk/vol1/ftp/technical/reference/human_g1k_v37.fasta.gz |
| Phasing & imputation | HG001 (NA12878) 1000 Genomes 30x CRAM (GRCh38) downsampled to 1x (2.1 GB) + 1000 Genomes high-coverage phased panel, 3,201 samples after excluding NA12878 (48 GB) | see below |
| Checksums (small files) | 20 GB of random small files | see below |

### Phasing and imputation data preparation

Following sections 1-2 of the GLIMPSE2 tutorial (https://odelaneau.github.io/GLIMPSE/docs/tutorials/getting_started/), applied to every autosome instead of chr22 only. Run from `GLIMPSE/tutorial/`.

Setup (tutorial section 1.1, builds the binaries and creates the `bin/` symlinks):

```bash
cd GLIMPSE/tutorial/
./step1_script_setup.sh
```

Reference panel: 1000 Genomes high-coverage phased panel (http://ftp.1000genomes.ebi.ac.uk/vol1/ftp/data_collections/1000G_2504_high_coverage/working/20201028_3202_phased/). Download, remove NA12878, normalize and keep bi-allelic SNPs, and extract the sites (tutorial section 2.1-2.3):

```bash
mkdir -p reference_panel
for CHR in $(seq 1 22); do
  PANEL=CCDG_14151_B01_GRM_WGS_2020-08-05_chr${CHR}.filtered.shapeit2-duohmm-phased.vcf.gz
  wget -c http://ftp.1000genomes.ebi.ac.uk/vol1/ftp/data_collections/1000G_2504_high_coverage/working/20201028_3202_phased/${PANEL}{,.tbi}

  bcftools norm -m -any ${PANEL} -Ou --threads 8 |
  bcftools view -m 2 -M 2 -v snps -s ^NA12878 --threads 8 -Ob -o reference_panel/1000GP.chr${CHR}.noNA12878.bcf
  bcftools index -f reference_panel/1000GP.chr${CHR}.noNA12878.bcf --threads 8
  rm ${PANEL}*

  bcftools view -G -Oz -o reference_panel/1000GP.chr${CHR}.noNA12878.sites.vcf.gz reference_panel/1000GP.chr${CHR}.noNA12878.bcf
  bcftools index -f reference_panel/1000GP.chr${CHR}.noNA12878.sites.vcf.gz
done
```

Target sample: NA12878 (HG001) whole-genome low-coverage (1x) BAM, `NA12878_1x_bam/NA12878.bam[.bai]` (2.1 GB), downsampled from the 1000 Genomes high-coverage (30x, GRCh38) CRAM of NA12878, `NA12878.final.cram`, listed in the 1000G_2504_high_coverage collection of the 1000 Genomes data portal (https://www.internationalgenome.org/data-portal/data-collection). ENA run ERR3239334: ftp://ftp.sra.ebi.ac.uk/vol1/run/ERR323/ERR3239334/NA12878.final.cram

```bash
# GRCh38 reference used by the 1000 Genomes CRAM files (needed to decode the CRAM)
wget -c http://ftp.1000genomes.ebi.ac.uk/vol1/ftp/technical/reference/GRCh38_reference_genome/GRCh38_full_analysis_set_plus_decoy_hla.fa{,.fai}

# NA12878 30x CRAM (and index) from the 1000G_2504_high_coverage collection
wget -c ftp://ftp.sra.ebi.ac.uk/vol1/run/ERR323/ERR3239334/NA12878.final.cram{,.crai}

# Downsample 30x -> 1x: keep 3.3% of the reads (-s SEED.FRACTION, the seed is arbitrary)
mkdir -p NA12878_1x_bam
samtools view -T GRCh38_full_analysis_set_plus_decoy_hla.fa -s 42.033 -b \
  -o NA12878_1x_bam/NA12878.bam NA12878.final.cram
samtools index NA12878_1x_bam/NA12878.bam
```

In our runs, GLIMPSE2 reported an average sequencing coverage of about 0.9x for this BAM.

Genetic maps: `GLIMPSE/maps/genetic_maps.b38/chr${CHR}.b38.gmap.gz`, included in the GLIMPSE repository.

### Checksum data (small files)

A set of about 20 GB of small files, similar in number and size distribution to an image dataset: 100,000 files with a log-normal size distribution (median about 180 KB, mean about 200 KB, total about 20 GB). File contents do not matter for checksums, so random data is used. The seed makes the file sizes reproducible.

```bash
mkdir -p small_files
awk -v n=100000 -v seed=42 'BEGIN {
  srand(seed); mu = log(200 * 1024) - 0.125; sigma = 0.5
  for (i = 1; i <= n; i++) {
    # Box-Muller: normal sample -> log-normal file size in bytes
    z = sqrt(-2 * log(1 - rand())) * cos(2 * 3.14159265 * rand())
    printf "%06d %d\n", i, int(exp(mu + sigma * z))
  }
}' | while read -r ID SIZE; do
  head -c "${SIZE}" /dev/urandom > small_files/file_${ID}.bin
done
du -sh small_files; ls small_files | wc -l
```

## Benchmarks (single node)

`NTHREADS=24` on the host and `NTHREADS=8` on a CSD.

### Quality control (FastQC)

```bash
time ./fastqc --threads ${NTHREADS} --noextract /mnt/nvme/bio/FASTQ/*
```

### Alignment (Minimap2)

The index is built from the reference at the start of the run and is included in the run time.

```bash
time minimap2 -t ${NTHREADS} -ax sr GCF_000001405.26_GRCh38_genomic.fna.gz \
  D1_S1_L001_R1_001.fastq.gz D1_S1_L001_R2_001.fastq.gz > aln.sam
```

### Variant calling (GraphTyper)

```bash
time ./graphtyper genotype human_g1k_v37.fasta \
  --sam=RMNISTHS_30xdownsample.bam --region=20 --threads=${NTHREADS}
```

### Phasing and imputation (GLIMPSE2)

Sections 3-5 of the GLIMPSE2 tutorial, for every autosome. Run from `GLIMPSE/tutorial/`.

Split the genome into chunks (tutorial section 3) and create the binary reference panel of each chunk (tutorial section 4):

```bash
mkdir -p reference_panel/split
for CHR in $(seq 1 22); do
  ./bin/GLIMPSE2_chunk --input reference_panel/1000GP.chr${CHR}.noNA12878.sites.vcf.gz --region chr${CHR} --output chunks.chr${CHR}.txt --map ../maps/genetic_maps.b38/chr${CHR}.b38.gmap.gz

  REF=reference_panel/1000GP.chr${CHR}.noNA12878.bcf
  MAP=../maps/genetic_maps.b38/chr${CHR}.b38.gmap.gz
  while IFS="" read -r LINE || [ -n "$LINE" ];
  do
    IRG=$(echo $LINE | cut -d" " -f3)
    ORG=$(echo $LINE | cut -d" " -f4)
    ./bin/GLIMPSE2_split_reference --reference ${REF} --map ${MAP} --input-region ${IRG} --output-region ${ORG} --output reference_panel/split/1000GP.chr${CHR}.noNA12878
  done < chunks.chr${CHR}.txt
done
```

```bash
#!/bin/bash
# step5c_script_impute.sh: tutorial section 5.1 over all autosomes, one GLIMPSE2_phase job per core
BAM=NA12878_1x_bam/NA12878.bam
OUT=GLIMPSE_impute/NA12878_imputed
THREADS=1                      # threads per GLIMPSE2_phase
N=$(( $(nproc) / THREADS ))    # number of parallel jobs
mkdir -p GLIMPSE_impute

for C in $(seq 1 22); do
  REF=reference_panel/split/1000GP.chr${C}.noNA12878
  while IFS="" read -r LINE || [ -n "$LINE" ];
  do
    IRG=$(echo $LINE | cut -d" " -f3)
    CHR=$(echo ${LINE} | cut -d" " -f2)
    REGS=$(echo ${IRG} | cut -d":" -f 2 | cut -d"-" -f1)
    REGE=$(echo ${IRG} | cut -d":" -f 2 | cut -d"-" -f2)
    while (( $(jobs -rp | wc -l) >= N )); do wait -n; done
    ./bin/GLIMPSE2_phase --bam-file ${BAM} --reference ${REF}_${CHR}_${REGS}_${REGE}.bin --output ${OUT}_${CHR}_${REGS}_${REGE}.bcf --threads ${THREADS} &
  done < chunks.chr${C}.txt
done
wait
```

Timed step (sections 3 and 4 of the tutorial are not included in the reported time):

```bash
time ./step5c_script_impute.sh
```

Note: Ligation (tutorial section 6) and concordance (tutorial section 7) can be used to verify the results.

### Checksums

`checksum_benchmark.sh` computes the checksum of every file in a directory, with one job per core. Set `CHECKSUM` to `md5sum` or `sha256sum` in the script.

```bash
./checksum_benchmark.sh /mnt/nvme/bio/small_files
```

```bash
#!/bin/bash

# Checksum program to use
#CHECKSUM=md5sum
CHECKSUM=sha256sum

# The number of parallel jobs, allocate as many as cpu cores
N=$(nproc)

# Take the first argument as input directory
INPUT_DIR="$1"

if [[ ! -d "${INPUT_DIR}" ]]
then
    echo "Directory $INPUT_DIR does not exist."
    exit 1
fi

WORK_TO_DO=()
while IFS= read -r -d '' file
do
    WORK_TO_DO+=("${CHECKSUM} \"${file}\"")
done < <(find "${INPUT_DIR}" -type f -print0)

RUNNING_JOBS=0
START=$(date +%s)

for CMD in "${WORK_TO_DO[@]}"
do
    if (( RUNNING_JOBS >= N ))
    then
        wait -n
        ((RUNNING_JOBS--))
    fi
    echo ${CMD}
    eval "${CMD} &"
    ((RUNNING_JOBS++))
done

wait
END=$(date +%s)
TOTAL=$((END - START))
echo "All checksums computed in ${TOTAL} seconds"
```

On-disk checksums on the CSD: `checksum_watcher.sh` (requires `inotify-tools`) watches a directory and writes a `.md5` file next to every new or modified file:

```bash
sudo apt install inotify-tools
./checksum_watcher.sh /mnt/nvme/data
```

## Distribution over host and CSDs with Slurm

The following job scripts are examples of how the benchmarks are distributed with Slurm.

The host and the CSDs are nodes of the same Slurm cluster (see the paper, Section 4.6). Each job runs a complete tool on a single node, on data stored on that node's drive. A node receives a new job as soon as it has finished its previous one.

Example `slurm.conf` excerpt, with the CSDs tagged by a feature so that jobs can be restricted to or excluded from them:

```
NodeName=host  CPUs=24 RealMemory=64000 Feature=x86_64
NodeName=csd[1-6] CPUs=8 RealMemory=15000 Feature=aarch64,csd
PartitionName=genomics Nodes=host,csd[1-6] Default=YES OverSubscribe=EXCLUSIVE
```

The job scripts use `$SLURM_CPUS_ON_NODE` so that the same script uses 24 threads on the host and 8 on a CSD. Paths must point to a location available on every node (the per-node drive, mounted at the same path; or the shared OCFS2 volume).

### QC: one job per FASTQ file (68 jobs)

```bash
#!/bin/bash
#SBATCH --job-name=fastqc
#SBATCH --partition=genomics
#SBATCH --exclusive
#SBATCH --array=0-67
FILES=(/mnt/nvme/bio/FASTQ/*.fastq.gz)
./fastqc --threads ${SLURM_CPUS_ON_NODE} --noextract "${FILES[$SLURM_ARRAY_TASK_ID]}"
```

### Alignment: one job per pair of FASTQ files

```bash
#!/bin/bash
#SBATCH --job-name=minimap2
#SBATCH --partition=genomics
#SBATCH --exclusive
# Submit with: sbatch --array=0-$(( $(ls /mnt/nvme/bio/FASTQ/*_R1_*.fastq.gz | wc -l) - 1 )) minimap2.sbatch
R1=(/mnt/nvme/bio/FASTQ/*_R1_*.fastq.gz)
F1="${R1[$SLURM_ARRAY_TASK_ID]}"
F2="${F1/_R1_/_R2_}"
minimap2 -t ${SLURM_CPUS_ON_NODE} -ax sr GCF_000001405.26_GRCh38_genomic.fna.gz \
  "${F1}" "${F2}" > aln/$(basename "${F1}" _R1_001.fastq.gz).sam
```

### Variant calling: one job per 1 Mb region of chromosome 20 (63 jobs)

```bash
#!/bin/bash
#SBATCH --job-name=graphtyper
#SBATCH --partition=genomics
#SBATCH --exclusive
#SBATCH --array=0-62
START=$(( SLURM_ARRAY_TASK_ID * 1000000 + 1 ))
END=$(( (SLURM_ARRAY_TASK_ID + 1) * 1000000 ))
(( SLURM_ARRAY_TASK_ID == 62 )) && END=63025520   # last region extends to the end of chromosome 20 (GRCh37)
./graphtyper genotype human_g1k_v37.fasta --sam=RMNISTHS_30xdownsample.bam \
  --region=20:${START}-${END} --threads=${SLURM_CPUS_ON_NODE} \
  --output=results/20_${START}_${END}
```

Note: Merging of the per-region VCF files (`bcftools concat`) is not included in the reported times.

### Phasing and imputation: one job per GLIMPSE2 chunk

```bash
#!/bin/bash
#SBATCH --job-name=glimpse2
#SBATCH --partition=genomics
#SBATCH --cpus-per-task=1     # keep equal to THREADS
# Submit with: sbatch --array=1-$(cat chunks.all.txt | wc -l) glimpse2.sbatch
THREADS=${SLURM_CPUS_PER_TASK:-1}
LINE=$(sed -n "${SLURM_ARRAY_TASK_ID}p" chunks.all.txt)
CHR=$(echo $LINE | cut -d" " -f2)
IRG=$(echo $LINE | cut -d" " -f3)
REGS=$(echo ${IRG} | cut -d":" -f2 | cut -d"-" -f1)
REGE=$(echo ${IRG} | cut -d":" -f2 | cut -d"-" -f2)
./bin/GLIMPSE2_phase --bam-file NA12878_1x_bam/NA12878.bam \
  --reference reference_panel/split/1000GP.${CHR}.noNA12878_${CHR}_${REGS}_${REGE}.bin \
  --output GLIMPSE_impute/NA12878_imputed_${CHR}_${REGS}_${REGE}.bcf --threads ${THREADS}
```

`chunks.all.txt` is the concatenation of the `chunks.chr*.txt` files. Slurm runs as many chunks in parallel on a node as it has cores divided by `--cpus-per-task`.

### Checksums: one job per file

```bash
#!/bin/bash
#SBATCH --job-name=checksum
#SBATCH --partition=genomics
#SBATCH --cpus-per-task=1
# Submit with: find /mnt/nvme/bio/small_files -type f > files.txt
#              sbatch --array=1-$(wc -l < files.txt) checksum.sbatch
FILE=$(sed -n "${SLURM_ARRAY_TASK_ID}p" files.txt)
sha256sum "${FILE}"      # or md5sum
```

For many small files, one array task per file adds Slurm overhead; grouping files per task (e.g., 100 files per task) is preferable.

### Restricting jobs to the host or to the CSDs

```bash
sbatch --constraint=csd    job.sbatch   # CSDs only
sbatch --constraint=x86_64 job.sbatch   # host only
```
