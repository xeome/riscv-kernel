#include "virtio.h"
#include "kernel.h"

/**
 * Reads a 32-bit value from a VirtIO device register at the specified offset.
 *
 * @param offset The offset of the register to read.
 * @return The value read from the register.
 */
uint32_t virtio_reg_read32(unsigned offset) {
    return *((volatile uint32_t*)(VIRTIO_BLK_PADDR + offset));
}

/**
 * Reads a 64-bit value from the specified offset in the VirtIO device's registers.
 *
 * @param offset The offset in bytes from the base address of the VirtIO device's registers.
 * @return The 64-bit value read from the specified offset.
 */
uint64_t virtio_reg_read64(unsigned offset) {
    return *((volatile uint64_t*)(VIRTIO_BLK_PADDR + offset));
}

/**
 * Writes a 32-bit value to a register in the VirtIO block device.
 *
 * @param offset The offset of the register to write to.
 * @param value The value to write to the register.
 */
void virtio_reg_write32(unsigned offset, uint32_t value) {
    *((volatile uint32_t*)(VIRTIO_BLK_PADDR + offset)) = value;
}

/**
 * Fetches the 32-bit value at the given offset in the VirtIO device's configuration space,
 * ORs it with the given value, and writes the result back to the same offset. This is
 * used to set feature bits in the device's configuration space.
 *
 * @param offset The offset of the 32-bit value to fetch, OR, and write back.
 * @param value The value to OR with the fetched 32-bit value.
 */
void virtio_reg_fetch_and_or32(unsigned offset, uint32_t value) {
    virtio_reg_write32(offset, virtio_reg_read32(offset) | value);
}

// blk_request_vq is a pointer to the virtio_virtq struct, which represents the virtual queue used for block requests.
struct virtio_virtq* blk_request_vq;
// blk_req is a pointer to the virtio_blk_req struct, which represents a block request.
struct virtio_blk_req* blk_req;
// blk_req_paddr is the physical address of the block request.
paddr_t blk_req_paddr;
// blk_capacity is an unsigned integer representing the capacity of the block device.
unsigned blk_capacity;

void virtio_blk_init(void) {
    if (virtio_reg_read32(VIRTIO_REG_MAGIC) != 0x74726976)
        PANIC("virtio: invalid magic value");
    if (virtio_reg_read32(VIRTIO_REG_VERSION) != 1)
        PANIC("virtio: invalid version");
    if (virtio_reg_read32(VIRTIO_REG_DEVICE_ID) != VIRTIO_DEVICE_BLK)
        PANIC("virtio: invalid device id");

    // 1. Reset the device.
    virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, 0);
    // 2. Set the ACKNOWLEDGE status bit: the guest OS has noticed the device.
    virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_ACK);
    // 3. Set the DRIVER status bit.
    virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER);
    // 5. Set the FEATURES_OK status bit.
    virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_FEAT_OK);
    // 7. Perform device-specific setup, including discovery of virtqueues for the device
    blk_request_vq = virtq_init(0);
    // 8. Set the DRIVER_OK status bit.
    virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER_OK);

    // Get capacity
    blk_capacity = virtio_reg_read64(VIRTIO_REG_DEVICE_CONFIG + 0) * SECTOR_SIZE;
    printf("virtio-blk: capacity is %d bytes\n", blk_capacity);

    // Allocate a page for blk_req
    blk_req_paddr = alloc_page(&page_list, align_up(sizeof(*blk_req), PAGE_SIZE) / PAGE_SIZE);
    blk_req = (struct virtio_blk_req*)blk_req_paddr;
}

struct virtio_virtq* virtq_init(unsigned index) {
    const paddr_t virtq_paddr = alloc_page(&page_list, align_up(sizeof(struct virtio_virtq), PAGE_SIZE) / PAGE_SIZE);
    struct virtio_virtq* vq = (struct virtio_virtq*)virtq_paddr;
    vq->queue_index = index;
    vq->used_index = (volatile uint16_t*)&vq->used.index;
    // 1. Select the queue writing its index (first queue is 0) to QueueSel.
    virtio_reg_write32(VIRTIO_REG_QUEUE_SEL, index);
    // 5. Notify the device about the queue size by writing the size to QueueNum.
    virtio_reg_write32(VIRTIO_REG_QUEUE_NUM, VIRTQ_ENTRY_NUM);
    // 6. Notify the device about the used alignment by writing its value in bytes to QueueAlign.
    virtio_reg_write32(VIRTIO_REG_QUEUE_ALIGN, 0);
    // 7. Write the physical number of the first page of the queue to the QueuePFN register.
    virtio_reg_write32(VIRTIO_REG_QUEUE_PFN, virtq_paddr);
    return vq;
}

/**
 * @brief Notifies the device that there are new requests available in the queue.
 *
 * @param vq Pointer to the virtio_virtq struct representing the queue.
 * @param desc_index The index of the descriptor that is now available.
 */
void virtq_kick(struct virtio_virtq* vq, int desc_index) {
    vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = desc_index;
    vq->avail.index++;
    __sync_synchronize();
    virtio_reg_write32(VIRTIO_REG_QUEUE_NOTIFY, vq->queue_index);
    vq->last_used_index++;
}

/**
 * Determines if a virtqueue is busy by comparing its last used index with its used index.
 *
 * @param vq The virtqueue to check.
 * @return True if the virtqueue is busy, false otherwise.
 */
bool virtq_is_busy(const struct virtio_virtq* vq) {
    return vq->last_used_index != *vq->used_index;
}
/**
 * @brief Reads or writes data to/from a disk sector using VirtIO block device.
 *
 * @param buf Pointer to the buffer to read/write data.
 * @param sector The sector number to read/write.
 * @param is_write Flag indicating whether to write data to the sector (1) or read data from the sector (0).
 */
void read_write_disk(void* buf, unsigned sector, int is_write) {
    // Check if the sector number is within the capacity of the block device.
    if (sector >= blk_capacity / SECTOR_SIZE) {
        printf("virtio: tried to read/write sector=%d, but capacity is %d\n", sector, blk_capacity / SECTOR_SIZE);
        return;
    }

    // Set the sector number and type of operation (read or write) in the block request.
    blk_req->sector = sector;
    blk_req->type = is_write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;

    // If writing to the sector, copy the data to the block request buffer.
    if (is_write)
        memcpy(blk_req->data, buf, SECTOR_SIZE);

    // Set up the descriptors for the VirtIO queue.
    struct virtio_virtq* vq = blk_request_vq;
    vq->descs[0].addr = blk_req_paddr;
    vq->descs[0].len = sizeof(uint32_t) * 2 + sizeof(uint64_t);
    vq->descs[0].flags = VIRTQ_DESC_F_NEXT;
    vq->descs[0].next = 1;

    vq->descs[1].addr = blk_req_paddr + offsetof(struct virtio_blk_req, data);
    vq->descs[1].len = SECTOR_SIZE;
    vq->descs[1].flags = VIRTQ_DESC_F_NEXT | (is_write ? 0 : VIRTQ_DESC_F_WRITE);
    vq->descs[1].next = 2;

    vq->descs[2].addr = blk_req_paddr + offsetof(struct virtio_blk_req, status);
    vq->descs[2].len = sizeof(uint8_t);
    vq->descs[2].flags = VIRTQ_DESC_F_WRITE;

    // Kick the VirtIO queue to start the operation.
    virtq_kick(vq, 0);

    // Wait for the operation to complete.
    while (virtq_is_busy(vq))
        ;

    // Check the status of the operation.
    if (blk_req->status != 0) {
        printf("virtio: warn: failed to read/write sector=%d status=%d\n", sector, blk_req->status);
        return;
    }

    // If reading from the sector, copy the data from the block request buffer to the output buffer.
    if (!is_write)
        memcpy(buf, blk_req->data, SECTOR_SIZE);
}