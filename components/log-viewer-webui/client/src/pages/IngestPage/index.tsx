import styles from "./index.module.css";
import { Col, Row } from 'antd';
import Size from "./Size";


/**
 * Presents compression statistics.
 *
 * @return
 */
const IngestPage = () => {
    return (
        <div className={styles["ingestPageGrid"]}>
            <Row>
                <Col span={8}><Size/></Col>
                <Col span={8}></Col>
                <Col span={8}></Col>
            </Row>
        </div>
    );
};


export default IngestPage;
