library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity PL_CONVERTER is
  Port( m_axis_tdata : in std_logic_vector(31 downto 0);
        m_axis_tkeep : in std_logic_vector(3 downto 0);
        m_axis_tlast : in std_logic;
        m_axis_tready : in std_logic;
        m_axis_tvalid : in std_logic;
        m_axis_tdata_out : out std_logic_vector(31 downto 0);
        m_axis_tkeep_out : out std_logic_vector(3 downto 0);
        m_axis_tlast_out : out std_logic;
        m_axis_tready_out : out std_logic;
        m_axis_tvalid_out : out std_logic);
end PL_CONVERTER;

architecture Behavioral of PL_CONVERTER is

    signal prag : std_logic_vector(31 downto 0) := x"00000080";

begin

    m_axis_tdata_out <= x"000000FF" when (m_axis_tdata > prag) else x"00000000";
    m_axis_tkeep_out <= m_axis_tkeep;
    m_axis_tlast_out <= m_axis_tlast;
    m_axis_tready_out <= m_axis_tready;
    m_axis_tvalid_out <= m_axis_tvalid;

end Behavioral;
