import { useEffect, useState } from 'react'
import { RefreshCw, Monitor, Globe, Activity, Search, WifiOff } from 'lucide-react'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { cn } from '@/lib/utils'
import MachineTable from '@/components/MachineTable'
import { fetchMachines } from '@/services/api'

// Une carte de statistique réutilisable (Total Agents, Actives, etc.)
function StatCard({ title, value, icon: Icon, description }) {
  return (
    <Card>
      <CardHeader className="flex flex-row items-center justify-between pb-2">
        <CardTitle className="text-xs font-semibold uppercase tracking-widest text-muted-foreground">
          {title}
        </CardTitle>
        <div className="flex h-7 w-7 items-center justify-center rounded-lg bg-muted">
          <Icon className="h-3.5 w-3.5 text-muted-foreground" />
        </div>
      </CardHeader>
      <CardContent>
        <div className="text-3xl font-bold">{value}</div>
        {description && <p className="mt-1 text-xs text-muted-foreground">{description}</p>}
      </CardContent>
    </Card>
  )
}

export default function Dashboard() {
  const [machines, setMachines] = useState([])
  const [loading, setLoading]   = useState(true)
  const [error, setError]       = useState(null)
  const [search, setSearch]     = useState('')

  // Charge les machines depuis l'API backend
  async function loadMachines() {
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

  // On charge les machines au premier affichage de la page
  useEffect(() => {
    loadMachines()
  }, [])

  // Machines vues dans les 10 dernières minutes → considérées "actives"
  const activeMachines = machines.filter((machine) => {
    if (!machine.last_seen) return false
    const tenMinutesMs = 10 * 60 * 1000
    return Date.now() - new Date(machine.last_seen).getTime() < tenMinutesMs
  }).length

  // Nombre d'IPs publiques uniques parmi toutes les machines
  const uniquePublicIPs = new Set(
    machines.map((m) => m.public_ip).filter(Boolean)
  ).size

  // Filtre les machines selon la saisie dans la barre de recherche
  const filteredMachines = machines.filter((machine) => {
    if (!search.trim()) return true
    const query = search.toLowerCase()
    return (
      machine.hostname?.toLowerCase().includes(query) ||
      machine.os_info?.toLowerCase().includes(query) ||
      machine.private_ip?.includes(query) ||
      machine.public_ip?.includes(query)
    )
  })

  return (
    <div className="flex flex-col gap-6 p-6">

      {/* En-tête de la page */}
      <div className="flex items-start justify-between gap-4">
        <div>
          <div className="flex items-center gap-3">
            <h1 className="text-xl font-semibold">Command &amp; Control</h1>
            {/* Badge de statut : vert = API OK, rouge = erreur */}
            <span className={cn(
              'flex items-center gap-1.5 rounded-full border px-2.5 py-0.5 text-xs font-semibold',
              error
                ? 'border-destructive/30 bg-destructive/10 text-destructive'
                : 'border-primary/30 bg-primary/10 text-primary'
            )}>
              <span className={cn(
                'h-1.5 w-1.5 rounded-full',
                error ? 'bg-destructive' : 'bg-primary animate-pulse'
              )} />
              {error ? 'Offline' : 'Live'}
            </span>
          </div>
          <p className="mt-1 text-sm text-muted-foreground">
            Liste des machines qui ont contacté le C2
          </p>
        </div>

        <Button variant="outline" size="sm" onClick={loadMachines} disabled={loading} className="gap-2">
          <RefreshCw className={cn('h-3.5 w-3.5', loading && 'animate-spin')} />
          Refresh
        </Button>
      </div>

      {/* Cartes de statistiques */}
      <div className="grid grid-cols-2 gap-4 lg:grid-cols-3">
        <StatCard
          title="Total Agents"
          value={loading ? '–' : machines.length}
          icon={Monitor}
          description="Machines enregistrées"
        />
        <StatCard
          title="Actives"
          value={loading ? '–' : activeMachines}
          icon={Activity}
          description="Vues dans les 10 dernières min"
        />
        <StatCard
          title="IPs Publiques"
          value={loading ? '–' : uniquePublicIPs}
          icon={Globe}
          description="Adresses externes uniques"
        />
      </div>

      {/* Tableau des machines */}
      <Card>
        <CardHeader className="pb-0">
          <div className="flex items-center justify-between gap-4">
            <div>
              <CardTitle className="text-sm font-semibold">Agent Roster</CardTitle>
              {!loading && (
                <p className="mt-0.5 text-xs text-muted-foreground">
                  {filteredMachines.length} machine{filteredMachines.length !== 1 ? 's' : ''}
                  {search && ` pour "${search}"`}
                </p>
              )}
            </div>

            {/* Barre de recherche */}
            <div className="relative w-64">
              <Search className="absolute left-2.5 top-1/2 h-3.5 w-3.5 -translate-y-1/2 text-muted-foreground pointer-events-none" />
              <Input
                placeholder="Rechercher hostname, IP..."
                value={search}
                onChange={(e) => setSearch(e.target.value)}
                className="pl-8 h-8 text-xs"
              />
            </div>
          </div>
        </CardHeader>

        <CardContent className="p-0 pt-4">
          {/* Message d'erreur si l'API est inaccessible */}
          {error ? (
            <div className="flex flex-col items-center gap-3 py-16 text-center">
              <WifiOff className="h-8 w-8 text-destructive opacity-50" />
              <p className="text-sm text-destructive">Backend inaccessible</p>
              <p className="text-xs text-muted-foreground">{error}</p>
              <Button variant="outline" size="sm" onClick={loadMachines} className="gap-2">
                <RefreshCw className="h-3.5 w-3.5" />
                Réessayer
              </Button>
            </div>
          ) : (
            <MachineTable machines={filteredMachines} loading={loading} />
          )}
        </CardContent>
      </Card>

    </div>
  )
}
