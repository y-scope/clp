import {
    Col,
    Row,
    Card,
    Typography,
} from "antd";



import styles from "./index.module.css";

const {Text} = Typography;

const Details = () => {
    return (
        <div className={styles["detailsGrid"]}>
            <Row gutter={[4,4]} className={styles["row"]}>
                <Col span={24} className={styles["col"]}>
                    <Card className={styles["card"] || ""} >
                        <div className={styles["cardContent"]}>
                            <Text className={styles["title"] || ""}>
                                Time Range
                            </Text>
                            <Text className={styles["statistic"] || ""}>
                                December 14, 2021 - April 16, 2025
                            </Text>
                        </div>
                    </Card>
                </Col>
                <Col span={12} className={styles["col"]}>
                    <Card className={styles["card"] || ""} >
                         <div className={styles["cardContent"]}>
                            <Text className={styles["title"] || ""}>
                                Messages
                            </Text>
                            <Text className={styles["statistic"] || ""}>
                                1235844
                            </Text>
                        </div>
                    </Card>
                </Col>
                <Col span={12} className={styles["col"]}>
                    <Card className={styles["card"] || ""} >
                    <div className={styles["cardContent"]}>
                            <Text className={styles["title"] || ""}>
                                Files
                            </Text>
                            <Text className={styles["statistic"] || ""}>
                                250
                            </Text>
                        </div>
                    </Card>
                </Col>
            </Row>
        </div>
    );
};

export default Details;
