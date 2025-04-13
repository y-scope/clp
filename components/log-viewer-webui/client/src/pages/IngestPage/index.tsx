import { Card, Typography, Divider, Table, Badge } from 'antd';
import { BarChart,Cell, Bar, XAxis, Tooltip, ResponsiveContainer, LabelList, Label, YAxis } from 'recharts';
import styles from './index.module.css';

const { Text } = Typography;

/**
 * Presents compression statistics.
 *
 * @return
 */
const IngestPage = () => {
    const value = 98.23; // Example value
    const data = [
        { name: 'Before', size: 80 },
        { name: 'After', size: 50 },
    ];

    const colors = ['#f5f5f5', '#FFA500']; // Colors for bars

    const TickLabel = ({ x, y, payload }) => {
        const index = data.findIndex((item) => item.name === payload.value); // Match the index based on the name
        return (
            <text x={x-80} y={y + 15} fill={colors[index]} className={styles['tickLabel']} textAnchor="middle" dominantBaseline="middle">
                {payload.value}
            </text>
        );
    };

    const BarLabel = ({ x, y, value, width, index }) => {
        return (
            <text x={x + width + 30} y={y + 20} fill={colors[index]} className={styles['barLabel']} textAnchor="start" dominantBaseline="middle">
                {value} MB
            </text>
        );
    };

    const columns = [
        { title: 'Name', dataIndex: 'name', key: 'name' },
        { title: 'Value', dataIndex: 'value', key: 'value' },
    ];

    const tableData = [
        { key: '1', name: 'Uncompressed Size', value: '80 MB' },
        { key: '2', name: 'Compressed Size', value: '50 MB' },
    ];

    const jobColumns = [
        {
            title: 'Ingestions Job Status',
            dataIndex: 'status',
            key: 'status',
            render: (status) => (
                <Badge
                    status={status === 'Completed' ? 'success' : 'processing'}
                    text={status}
                />
            ),
        },
        { title: 'Job ID', dataIndex: 'jobId', key: 'jobId' },
        { title: 'Speed', dataIndex: 'speed', key: 'speed' },
        { title: 'Data Ingested', dataIndex: 'dataIngested', key: 'dataIngested' },
        { title: 'Compressed Size', dataIndex: 'compressedSize', key: 'compressedSize' },
    ];

    const jobData = [
        { key: '1', status: 'Completed', jobId: '12345', speed: '10 MB/s', dataIngested: '500 MB', compressedSize: '300 MB' },
        { key: '2', status: 'In Progress', jobId: '12346', speed: '8 MB/s', dataIngested: '400 MB', compressedSize: '250 MB' },
    ];

    return (
        <div className={styles['ingestPage']}>
            <Card
                className={styles['SpaceCard'] || ""}
                title="Space Savings" // Renamed title
                hoverable>
                <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-start' }}>
                    <div>
                        <Text className={styles['hello'] || ""}>
                            {value.toFixed(2)}%
                        </Text>
                    </div>
                    <div style={{ marginTop: '20px' }}>
                        <BarChart data={data} width={550} height={150} layout="vertical" margin={{ top: 0, right: 60, left: 110, bottom: 5 }}>
                            <YAxis
                                type="category"
                                dataKey="name"
                                tickLine={false}
                                axisLine={false}
                                tick={(props) => <TickLabel {...props} index={data.findIndex((item) => item.name === props.payload.value)} />}
                            />
                            <XAxis type="number" axisLine={false } tickLine={false} hide/>
                            <Tooltip />
                            <Bar dataKey="size" barSize={30} radius={[0, 10, 10, 0]}>
                            {
                                data.map((entry, index) => (
                                    <Cell key={`cell-${index}`} fill={colors[index]} />
                                ))
                            }
                                <LabelList
                                    dataKey="size"
                                    content={(props) => <BarLabel {...props} index={props.index} />}
                                />
                            </Bar>
                        </BarChart>
                    </div>
                </div>
            </Card>
            <Card
                className={styles['DetailsCard'] || ""}
                title="Details"
                hoverable>
                {/* Empty card for now */}
            </Card>
            <Card
                className={styles['Ingestions Jobs'] || ""}
                title="Ingestions Jobs"
                hoverable
                style={{ gridColumn: '1 / -1' }} // Make the card span the full width
            >
                <Table columns={jobColumns} dataSource={jobData} pagination={false} />
            </Card>
        </div>
    );
};

export default IngestPage;
