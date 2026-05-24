import { useEffect, useState } from 'react'
import { RefreshCw, Monitor, Globe, Activity } from 'lucide-react'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import MachineTable from '@/components/MachineTable'
import { fetchMachines } from '@/services/api'

function StatCard({ title, value, icon: Icon, description }) {
  return (
    <Card>
      <CardHeader className="flex flex-row items-center justify-between pb-2">
        <CardTitle className="text-sm font-medium text-muted-foreground">{title}</CardTitle>
        <Icon className="h-4 w-4 text-muted-foreground" />
      </CardHeader>
      <CardContent>
        <div className="text-2xl font-bold">{value}</div>
        {description && (
          <p className="text-xs text-muted-foreground mt-1">{description}</p>
        )}
      </CardContent>
    </Card>
  )
}

export default function Dashboard() {
  const [machines, setMachines] = useState([])
  const [loading, setLoading]   = useState(true)
  const [error, setError]       = useState(null)

  const load = async () => {
    setLoading(true)
    setError(null)
    try {
      const data = await fetchMachines()
      setMachines(data)
    } catch (err) {
      setError(err.message)
    } finally {
      setLoading(false)
    }
  }

  useEffect(() => { load() }, [])

  const uniqueIPs = new Set(machines.map((m) => m.public_ip).filter(Boolean)).size

  return (
    <div className="flex flex-col gap-6 p-6">
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-xl font-semibold">Dashboard</h1>
          <p className="text-sm text-muted-foreground">Infected machines overview</p>
        </div>
        <Button variant="outline" size="sm" onClick={load} disabled={loading}>
          <RefreshCw className={`h-4 w-4 ${loading ? 'animate-spin' : ''}`} />
          Refresh
        </Button>
      </div>

      <div className="grid grid-cols-3 gap-4">
        <StatCard
          title="Total Machines"
          value={machines.length}
          icon={Monitor}
          description="All reporting agents"
        />
        <StatCard
          title="Unique Public IPs"
          value={uniqueIPs}
          icon={Globe}
          description="Distinct external addresses"
        />
        <StatCard
          title="Status"
          value={error ? 'Error' : 'Online'}
          icon={Activity}
          description={error ?? 'API reachable'}
        />
      </div>

      <Card>
        <CardHeader>
          <CardTitle className="text-base">Machines</CardTitle>
        </CardHeader>
        <CardContent className="p-0">
          {error ? (
            <p className="p-6 text-sm text-destructive">{error}</p>
          ) : (
            <MachineTable machines={machines} loading={loading} onRefresh={load} />
          )}
        </CardContent>
      </Card>
    </div>
  )
}
