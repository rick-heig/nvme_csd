#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

const char *path = "/sys/kernel/config/pci_ep/functions/pci_epf_nvme/pci_epf_nvme.0/nvme/";

int main() {
	printf("Reading from %s\n", path);
	size_t buffer_size = 64000;
	int ret;
	char *rd_buffer;
	char *wr_buffer;
	FILE *fp;

	fp = fopen("/sys/kernel/config/pci_ep/functions/pci_epf_nvme/pci_epf_nvme.0/nvme/statistics_buffer_size", "r");
	// This is not necessarily available
	if (fp) {
		ret = fscanf(fp, "%zu", &buffer_size);
		printf("Buffer is of size %zu\n", buffer_size);
		fclose(fp);
	}

	if (!buffer_size) {
		printf("Will not read a buffer of size 0 !\n");
		return -1;
	}

	rd_buffer = malloc(buffer_size);
	wr_buffer = malloc(buffer_size);

	if (!rd_buffer || !wr_buffer) {
		printf("Could not allocate buffers !\n");
		return -1;
	}

	fp = fopen("/sys/kernel/config/pci_ep/functions/pci_epf_nvme/pci_epf_nvme.0/nvme/rd_statistics", "rb");
	if (!fp) {
		printf("Could not open file\n");
		return -1;
	}
	ret = fread(rd_buffer, buffer_size, 1, fp);

	if (ret < 0) {
		printf("Could not fill buffer, ret = %d\n", ret);
		return -1;
	}
	fclose(fp);

	fp = fopen("/sys/kernel/config/pci_ep/functions/pci_epf_nvme/pci_epf_nvme.0/nvme/wr_statistics", "rb");
	if (!fp) {
		printf("Could not open file\n");
		return -1;
	}
	ret = fread(wr_buffer, buffer_size, 1, fp);

	if (ret < 0) {
		printf("Could not fill buffer, ret = %d\n", ret);
		return -1;
	}
	fclose(fp);

	fp = fopen("binary_dump_rd_stats.bin", "wb");
	if (!fp) {
		printf("Could not open output file\n");
		return -1;
	}
	fwrite(rd_buffer, buffer_size, 1, fp);
	fclose(fp);

	fp = fopen("binary_dump_wr_stats.bin", "wb");
	if (!fp) {
		printf("Could not open output file\n");
		return -1;
	}
	fwrite(wr_buffer, buffer_size, 1, fp);
	fclose(fp);

	free(rd_buffer);
	free(wr_buffer);

	return 0;
}
