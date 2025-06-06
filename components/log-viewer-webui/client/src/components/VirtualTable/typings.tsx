import type {TableProps} from "antd";


/**
 * Amount of pixels to scroll when using keyboard navigation.
 */
export const SCROLL_INCREMENT = 32;

/**
 * CSS selector for the virtual table body element.
 */
export const VIRTUAL_TABLE_HOLDER_SELECTOR = ".ant-table-tbody-virtual-holder";

/**
 * Antd Table props with virtual omitted since set by VirtualTable.
 */
export type VirtualTableProps<RecordType> = Omit<TableProps<RecordType>, "virtual">;
