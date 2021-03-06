defmodule Ockam.Tests do
  use ExUnit.Case, async: true
  doctest Ockam

  describe "Ockam.start/2" do
    test "Ockam.Routing is started", do: Ockam.Routing |> find_child |> Process.alive?() |> assert
    test "Ockam.Control is started", do: Ockam.Control |> find_child |> Process.alive?() |> assert
  end

  def find_child(name) do
    {_, pid, _, _} =
      Ockam |> Supervisor.which_children() |> Enum.find(fn {t, _, _, _} -> t == name end)

    pid
  end
end
