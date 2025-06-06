import React, { useRef, useCallback } from "react";
import { Table } from "antd";
import type { TableProps } from "antd";

/**
 * Amount of pixels to scroll when using keyboard navigation.
 */
const SCROLL_INCREMENT = 32;

/**
 * CSS selector for the virtual table body element.
 */
const VIRTUAL_TABLE_HOLDER_SELECTOR = '.ant-table-tbody-virtual-holder';

/**
 * Props for VirtualTable. Omits both virtual and scroll antd props since they're set by this component.
 */
type VirtualTableProps<RecordType> = Omit<TableProps<RecordType>, 'virtual' | 'scroll'> & {
    /**
     * Height of the virtual table's scrollable area.
     */
  scrollY: number | string;
  ref?: React.Ref<any>;
};

/**
 * Virtual table that supports keyboard navigation.
 */
const VirtualTable = <RecordType extends object = any>({
    scrollY,
    ref,
    ...tableProps
}: VirtualTableProps<RecordType>) => {
    const containerRef = useRef<HTMLDivElement>(null);

    const handleKeyDown = useCallback((e: React.KeyboardEvent<HTMLDivElement>) => {
        if (!containerRef.current) return;

        const scrollNode = containerRef.current.querySelector<HTMLElement>(VIRTUAL_TABLE_HOLDER_SELECTOR);
        if (!scrollNode) return;

        const visibleTableHeight = scrollNode.clientHeight;
        let scrollTop = scrollNode.scrollTop;

        switch (e.key) {
            case "ArrowDown":
                scrollTop += SCROLL_INCREMENT;
                break;
            case "ArrowUp":
                // Prevent scrolling past the top.
                scrollTop = Math.max(scrollTop - SCROLL_INCREMENT, 0);
                break;
            case "PageDown":
                scrollTop += visibleTableHeight;
                break;
            case "PageUp":
                // Prevent scrolling past the top.
                scrollTop = Math.max(scrollTop - visibleTableHeight, 0);
                break;
            case "Home":
                scrollTop = 0;
                break;
            case "End":
                // Scroll to the bottom of the table.
                scrollTop = Number.MAX_SAFE_INTEGER;
                break;
            default:
                return;
        }

        scrollNode.scrollTop = scrollTop;
        e.preventDefault();
    }, []);

    return (
        <div
            ref={containerRef}
            tabIndex={0}
            onKeyDown={handleKeyDown}
        >
            <Table<RecordType>
                ref={ref}
                virtual
                scroll={{ y: scrollY }}
                {...tableProps}
            />
        </div>
    );
};

export default VirtualTable;
