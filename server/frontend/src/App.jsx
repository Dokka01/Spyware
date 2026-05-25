import { BrowserRouter as Router, Route, Routes } from 'react-router-dom'
import Sidebar from '@/components/layout/Sidebar'
import Dashboard from '@/pages/Dashboard'
import Settings from '@/pages/Settings'
import MachineDetail from '@/pages/MachineDetail'

export default function App() {
  return (
    <Router>
      <div className="flex h-screen overflow-hidden bg-background">
        <Sidebar />
        <main className="flex-1 overflow-y-auto">
          <Routes>
            <Route path="/"            element={<Dashboard />} />
            <Route path="/machine/:id" element={<MachineDetail />} />
            <Route path="/settings"    element={<Settings />} />
          </Routes>
        </main>
      </div>
    </Router>
  )
}
