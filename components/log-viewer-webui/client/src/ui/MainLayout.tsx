import React, { useState } from 'react';
import {
  UploadOutlined,
  SearchOutlined,
} from '@ant-design/icons';
import type { MenuProps } from 'antd';
import { Layout, Menu } from 'antd';
import './MainLayout.css';

const {Sider } = Layout;

type MenuItem = Required<MenuProps>['items'][number];

const items: MenuItem[] = [
    {label: "Ingest", key: "/ingest", icon: <UploadOutlined />},
    {label: "Search", key: "/search", icon: <SearchOutlined />},
];

const MainLayout: React.FC = () => {
  const [collapsed, setCollapsed] = useState(false);
  
  return (
    <Layout className="main-layout">
      <Sider
        collapsible
        collapsed={collapsed}
        onCollapse={(value) => setCollapsed(value)}
        theme="light"
        width="150"
      >
        <div className="sider-logo-container">
          <img src="/clp-logo.png" alt="CLP Logo" className="sider-logo" />
        </div>
        <Menu defaultSelectedKeys={['1']} mode="inline" items={items} />
      </Sider>
    </Layout>
  );
};

export default MainLayout;