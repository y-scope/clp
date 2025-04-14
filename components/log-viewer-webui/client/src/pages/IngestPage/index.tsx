import Size from './Size';
import styles from './index.module.css';

/**
 * Presents compression statistics.
 *
 * @return
 */

const IngestPage = () => {
    return (
        <div className={styles["ingestPageGrid"]}>
            <Size />
        </div>
    );
};

export default IngestPage;