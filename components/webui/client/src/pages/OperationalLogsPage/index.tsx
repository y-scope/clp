import {
    useEffect,
    useMemo,
    useRef,
    useState,
} from "react";

import {ReloadOutlined} from "@ant-design/icons";
import {useQuery} from "@tanstack/react-query";
import {
    Button,
    Empty,
    Select,
    Spin,
    Typography,
} from "antd";
import type {ColumnsType} from "antd/es/table";

import {
    listComponents,
    listLogFiles,
    readLogContent,
} from "../../api/operational-logs";
import VirtualTable from "../../components/VirtualTable";
import styles from "./index.module.css";


const {Text, Title} = Typography;

// Padding accounts for: page bottom padding (16px) + table header (~39px) + buffer
const TABLE_BOTTOM_PADDING = 60;

interface LogTableEntry {
    key: string;
    timestamp: number;
    source: string;
    log: string;
}

/**
 * Formats a Unix timestamp to a readable date/time string.
 *
 * @param timestamp Unix timestamp in seconds
 * @return Formatted date/time string
 */
const formatTimestamp = (timestamp: number): string => {
    const date = new Date(timestamp * 1000);
    return date.toISOString().replace("T", " ")
        .slice(0, 23);
};

/**
 * Operational Logs page for viewing CLP component logs.
 *
 * @return
 */
const OperationalLogsPage = () => {
    const [selectedComponent, setSelectedComponent] = useState<string | undefined>(undefined);
    const [selectedFile, setSelectedFile] = useState<string | undefined>(undefined);
    const [tableHeight, setTableHeight] = useState<number>(400);
    const tableContainerRef = useRef<HTMLDivElement>(null);

    // Calculate table height based on available space
    useEffect(() => {
        const updateHeight = () => {
            if (tableContainerRef.current) {
                const {top} = tableContainerRef.current.getBoundingClientRect();
                const availableHeight = window.innerHeight - top - TABLE_BOTTOM_PADDING;
                setTableHeight(Math.max(200, availableHeight));
            }
        };

        updateHeight();
        window.addEventListener("resize", updateHeight);

        return () => {
            window.removeEventListener("resize", updateHeight);
        };
    }, []);

    // Fetch available components
    const {
        data: components = [],
        isLoading: isLoadingComponents,
        refetch: refetchComponents,
    } = useQuery({
        queryKey: ["operational-logs",
            "components"],
        queryFn: listComponents,
    });

    // Fetch log files for selected component
    const {
        data: logFiles = [],
        isLoading: isLoadingFiles,
        refetch: refetchFiles,
    } = useQuery({
        queryKey: ["operational-logs",
            "files",
            selectedComponent],
        queryFn: () => listLogFiles(selectedComponent),
        enabled: true,
    });

    // Fetch log content for selected file
    const {
        data: logContent,
        isLoading: isLoadingContent,
        refetch: refetchContent,
    } = useQuery({
        queryKey: ["operational-logs",
            "content",
            selectedFile],
        queryFn: () => {
            if ("undefined" === typeof selectedFile) {
                return Promise.resolve({entries: [], totalLines: 0, hasMore: false});
            }

            return readLogContent(selectedFile, 0, 1000);
        },
        enabled: "undefined" !== typeof selectedFile,
    });

    // Auto-select first file when component changes
    useEffect(() => {
        if (0 < logFiles.length) {
            const relevantFiles = "undefined" !== typeof selectedComponent ?
                logFiles.filter((f) => f.component === selectedComponent) :
                logFiles;

            if (0 < relevantFiles.length) {
                setSelectedFile(relevantFiles[0]?.path);
            }
        }
    }, [logFiles,
        selectedComponent]);

    // Create component options
    const componentOptions = useMemo(() => {
        return components.map((c) => ({
            label: `${c.name}${c.hasLogs ?
                "" :
                " (no logs)"}`,
            value: c.name,
            disabled: false === c.hasLogs,
        }));
    }, [components]);

    // Create file options
    const fileOptions = useMemo(() => {
        const relevantFiles = "undefined" !== typeof selectedComponent ?
            logFiles.filter((f) => f.component === selectedComponent) :
            logFiles;

        return relevantFiles.map((f) => ({
            label: `${f.component}/${f.filename}`,
            value: f.path,
        }));
    }, [logFiles,
        selectedComponent]);

    // Create table data
    const tableData = useMemo((): LogTableEntry[] => {
        if ("undefined" === typeof logContent) {
            return [];
        }

        return logContent.entries.map((entry, index) => ({
            key: `${entry.timestamp}-${index}`,
            timestamp: entry.timestamp,
            source: entry.source,
            log: entry.log,
        }));
    }, [logContent]);

    // Define table columns
    const columns: ColumnsType<LogTableEntry> = [
        {
            title: "Timestamp",
            dataIndex: "timestamp",
            key: "timestamp",
            width: 200,
            render: (timestamp: number) => (
                <Text className={styles["logTimestamp"] ?? ""}>
                    {formatTimestamp(timestamp)}
                </Text>
            ),
        },
        {
            title: "Source",
            dataIndex: "source",
            key: "source",
            width: 80,
            render: (source: string) => (
                <Text
                    className={`${styles["logSource"] ?? ""} ${"stderr" === source ?
                        styles["logSourceStderr"] ?? "" :
                        ""}`}
                >
                    {source}
                </Text>
            ),
        },
        {
            title: "Message",
            dataIndex: "log",
            key: "log",
            render: (log: string) => (
                <Text className={styles["logMessage"] ?? ""}>
                    {log}
                </Text>
            ),
        },
    ];

    const handleRefresh = () => {
        refetchComponents();
        refetchFiles();
        if ("undefined" !== typeof selectedFile) {
            refetchContent();
        }
    };

    const isLoading = isLoadingComponents || isLoadingFiles || isLoadingContent;

    return (
        <div className={styles["operationalLogsPage"] ?? ""}>
            <div className={styles["header"] ?? ""}>
                <Title
                    className={styles["pageTitle"] ?? ""}
                    level={4}
                >
                    {"undefined" !== typeof selectedFile ?
                        `Log entries (${logContent?.totalLines ?? 0} total)` :
                        "Operational Logs"}
                </Title>
                <Select
                    allowClear={true}
                    className={styles["componentSelect"] ?? ""}
                    disabled={isLoadingComponents}
                    loading={isLoadingComponents}
                    options={componentOptions}
                    placeholder={"All components"}
                    value={selectedComponent}
                    onChange={setSelectedComponent}/>
                <Select
                    className={styles["fileSelect"] ?? ""}
                    disabled={isLoadingFiles || 0 === fileOptions.length}
                    loading={isLoadingFiles}
                    options={fileOptions}
                    placeholder={"Select a log file"}
                    value={selectedFile}
                    onChange={setSelectedFile}/>
                <Button
                    className={styles["refreshButton"] ?? ""}
                    icon={<ReloadOutlined/>}
                    loading={isLoading}
                    type={"default"}
                    onClick={handleRefresh}
                >
                    Refresh
                </Button>
            </div>

            <div
                className={styles["tableContainer"] ?? ""}
                ref={tableContainerRef}
            >
                {isLoading ?
                    (
                        <div className={styles["emptyState"] ?? ""}>
                            <Spin size={"large"}/>
                        </div>
                    ) :
                    0 === tableData.length ?
                        (
                            <Empty
                                className={styles["emptyState"] ?? ""}
                                description={"No log entries found"}/>
                        ) :
                        (
                            <VirtualTable<LogTableEntry>
                                className={styles["logTable"] ?? ""}
                                columns={columns}
                                dataSource={tableData}
                                pagination={false}
                                scroll={{y: tableHeight}}
                                size={"small"}/>
                        )}
            </div>
        </div>
    );
};

export default OperationalLogsPage;
